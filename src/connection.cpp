#include "connection.hpp"
#include "server.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

Connection::Connection(tcp::socket socket, ThreadPool& thread_pool)
    : socket_(std::move(socket))
    , thread_pool_(thread_pool) {}

void Connection::start_monitoring(std::function<void(const Event&)> enqueue_callback) {
    enqueue_callback_ = std::move(enqueue_callback);
    async_read(); // READ 이벤트 감지 시작
}

void Connection::async_read() {
    read_chunk(); // 8바이트 단위 읽기 시작
}

void Connection::read_chunk() {
    auto chunk_buffer = std::make_shared<std::vector<char>>(8); // 항상 8바이트씩 읽기
    auto self = shared_from_this();

    std::cout << "[DEBUG] Starting to read a chunk of 8 bytes.\n";

    boost::asio::async_read(socket_, boost::asio::buffer(*chunk_buffer),
        [this, self, chunk_buffer](const boost::system::error_code& ec, std::size_t bytes_transferred) {
            if (!ec && bytes_transferred == 8) {
                std::cout << "[DEBUG] Successfully read 8 bytes. Data: ";
                for (char c : *chunk_buffer) {
                    std::cout << std::hex << static_cast<int>(c & 0xFF) << " ";
                }
                std::cout << std::endl;

                try {
                    if (is_header(*chunk_buffer)) {
                        std::cout << "[DEBUG] Chunk identified as header.\n";
                        Header header = parse_header(*chunk_buffer);
                        std::cout << "[DEBUG] Parsed Header: type=" << static_cast<int>(header.type)
                                  << ", body_length=" << header.body_length << "\n";

                        read_body(header);
                    } else {
                        throw std::runtime_error("Invalid header format");
                    }
                } catch (const std::exception& e) {
                    std::cerr << "[ERROR] Exception while parsing header: " << e.what() << std::endl;
                    enqueue_callback_(Event{
                        EventType::ERROR,
                        self,
                        RequestType::UNKNOWN,
                        std::vector<char>(e.what(), e.what() + std::strlen(e.what()))
                    });
                }
            } else if (ec == boost::asio::error::eof || ec == boost::asio::error::connection_reset) {
                // 연결 종료 또는 소켓 클로즈 감지
                std::cout << "[DEBUG] Connection closed by client.\n";
                enqueue_callback_(Event{
                    EventType::CLOSE,
                    self,
                    RequestType::UNKNOWN,
                    {}
                });
            } else {
                std::cerr << "[ERROR] Failed to read 8 bytes. Error: " 
                          << (ec ? ec.message() : "Unknown error") << std::endl;
                enqueue_callback_(Event{
                    EventType::ERROR,
                    self,
                    RequestType::UNKNOWN,
                    {}
                });
            }
        });
}

bool Connection::is_header(const std::vector<char>& buffer) {
    if (buffer.size() < 8) {
        std::cout << "[DEBUG] Buffer size is too small to be a header.\n";
        return false;
    }

    RequestType type = static_cast<RequestType>(buffer[0]);
    uint32_t body_length = 0;

    for (int i = 0; i < 4; ++i) {
        body_length |= (static_cast<uint32_t>(buffer[1 + i]) << (i * 8));
    }

    std::cout << "[DEBUG] Checking if buffer is header. Type=" << static_cast<int>(type)
              << ", Body length=" << body_length << "\n";

    return (type == RequestType::IN || type == RequestType::OUT) && body_length > 0;
}

Header Connection::parse_header(const std::vector<char>& buffer) {
    if (buffer.size() < 8) {
        throw std::invalid_argument("Buffer size is too small to parse Header.");
    }

    Header header;
    header.type = static_cast<RequestType>(buffer[0]);
    header.body_length = 0;

    // 리틀 엔디언으로 body_length 파싱
    for (int i = 0; i < 4; ++i) {
        header.body_length |= (static_cast<uint32_t>(buffer[1 + i]) << (i * 8));
    }

    // 디버깅 출력
    std::cout << "[DEBUG] Parsed Header: Type=" << static_cast<int>(header.type)
              << ", Body Length=" << header.body_length << std::endl;

    return header;
}

void Connection::read_body(const Header& header) {
    if (header.body_length > 10 * 1024 * 1024) { // 최대 크기 10MB
        std::cerr << "[ERROR] Body length exceeds maximum allowed size: " << header.body_length << " bytes.\n";
        enqueue_callback_(Event{EventType::ERROR, shared_from_this(), RequestType::UNKNOWN, {}});
        return;
    }

    size_t padded_size = ((header.body_length + 7) / 8) * 8; // 패딩 포함 크기 (8의 배수)
    std::cout << "[DEBUG] Starting to read body. Padded size=" << padded_size 
              << ", Actual body length=" << header.body_length << "\n";

    auto body_buffer = std::make_shared<std::vector<char>>(padded_size);
    read_body_chunk(body_buffer, header, 0);
}


void Connection::read_body_chunk(std::shared_ptr<std::vector<char>> body_buffer, const Header& header, std::size_t bytes_read) {
    auto self = shared_from_this();
    size_t remaining = body_buffer->size() - bytes_read;

    std::cout << "[DEBUG] Reading body chunk. Bytes read so far=" << bytes_read 
              << ", Remaining=" << remaining << "\n";

    boost::asio::async_read(socket_, boost::asio::buffer(body_buffer->data() + bytes_read, remaining),
        [this, self, body_buffer, header, bytes_read](const boost::system::error_code& ec, std::size_t bytes_transferred) {
            if (!ec) {
                std::size_t total_bytes_read = bytes_read + bytes_transferred;
                std::cout << "[DEBUG] Successfully read body chunk. Total bytes read=" << total_bytes_read << "\n";

                if (total_bytes_read < body_buffer->size()) {
                    // 계속 읽음
                    read_body_chunk(body_buffer, header, total_bytes_read);
                } else {
                    // 본문 읽기 완료
                    std::cout << "[DEBUG] Body read complete. Removing padding.\n";
                    Event ev;
                    ev.type = EventType::READ;
                    ev.connection = self;
                    ev.request_type = header.type;

                    // 패딩 제거
                    ev.data = std::vector<char>(body_buffer->begin(), body_buffer->begin() + header.body_length);
                    enqueue_callback_(ev);

                    // 다음 수신 대기
                    async_read();
                }
            } else {
                std::cerr << "[ERROR] Error while reading body chunk. Error: " 
                          << ec.message() << "\n";
                enqueue_callback_(Event{EventType::CLOSE, self, {}});
            }
        });
}

void Connection::async_write(const std::string& data) {
    auto buffer = std::make_shared<std::string>(data);
    auto self = shared_from_this();

    boost::asio::async_write(socket_, boost::asio::buffer(*buffer),
        [this, self, buffer](const boost::system::error_code& ec, std::size_t) {
            if (!ec) {
                enqueue_callback_(Event{EventType::WRITE, self, {}});
            } else {
                enqueue_callback_(Event{EventType::CLOSE, self, {}});
            }
        });
}

std::vector<char> serialize_header(const Header& header) {
    std::vector<char> buffer(8);
    std::memcpy(buffer.data(), &header, sizeof(Header));
    std::cout << "[DEBUG] Serialized Header: ";
    for (char c : buffer) {
        std::cout << std::hex << static_cast<int>(c & 0xFF) << " ";
    }
    std::cout << std::endl;
    return buffer;
}

std::string Connection::create_response_string(RequestType type, const std::string& body) {
    // 헤더 생성
    Header header{type, static_cast<uint32_t>(body.size())};

    std::cout << "-----Size of Header: " << sizeof(Header) << std::endl;
    serialize_header(header);

    // 헤더를 바이너리 데이터로 변환
    std::vector<char> header_buffer(sizeof(Header));
    std::memcpy(header_buffer.data(), &header, sizeof(Header));

    // 본문 데이터 패딩 처리 (8바이트 배수로 맞춤)
    size_t padded_body_length = ((body.size() + 7) / 8) * 8; // 8의 배수로 맞춤
    std::string padded_body = body;
    padded_body.resize(padded_body_length, '\0'); // '\0'으로 패딩 추가

    // 헤더와 패딩된 본문 결합
    std::string response(header_buffer.begin(), header_buffer.end());
    response += padded_body;

    return response; // 결합된 헤더 + 패딩된 본문 스트링 반환
}

void Connection::onRead(const std::vector<char>& data, RequestType type) { 
    try {
        std::string message(data.begin(), data.end());
        std::cout << "Received data: " << message << std::endl;

        // 요청 타입에 따라 분기 처리
        switch (type) {
        case RequestType::IN:
        {            
            // 작업을 람다 함수로 정의
            thread_pool_.enqueue_task([this, data]() {

                // json 데이터 가정(필수)
                // {
                //     "player_id": "12345",
                //     "player_name": "Alice"
                // }

                // JSON 파싱
                json parsed_data = json::parse(data);
                // 클라이언트에서 보낸 데이터에서 플레이어 정보 추출
                std::string player_id = parsed_data["player_id"].get<std::string>();
                std::string player_name = parsed_data["player_name"].get<std::string>();
                
                // 새로운 플레이어 객체 생성
                auto player = std::make_shared<Player>(player_id, player_name);

                // 서버 싱글톤 인스턴스 가져오기
                Server& server = Server::getInstance();
                // 룸에 플레이어 추가
                server.add_player_to_room(player);

                // connection_manager에 관계 등록
                ConnectionManager& cm = server.get_connection_manager();
                cm.register_connection(player, shared_from_this());

                // 방에 참가한 모든 플레이어에게 전달될 데이터 예시
                // {
                //     "room_id": 1,
                //     "players": [
                //         {
                //             "id": "player1",
                //             "name": "Alice"
                //         },
                //         {
                //             "id": "player2",
                //             "name": "Bob"
                //         }
                //     ]
                // }

                // 플레이어에게 룸 정보 전송
                auto room = player->room; // 플레이어가 속한 룸
                if (room) {
                    json room_info;
                    room_info["room_id"] = room->id;
                    room_info["players"] = json::array();

                    for (const auto& p : room->get_players()) {
                        room_info["players"].push_back({
                            {"id", p->id},    // 플레이어 ID
                            {"name", p->name} // 플레이어 이름
                        });
                    }

                    std::string response = create_response_string(RequestType::IN, room_info.dump());

                    room->broadcast_message(response); // 방 전체에 룸 정보 브로드캐스트
                }

            });

            break;
        }
        case RequestType::OUT:
        {
            thread_pool_.enqueue_task([this, data]() {

                // 서버 싱글톤 인스턴스 가져오기
                Server& server = Server::getInstance();
                ConnectionManager& connection_manager = server.get_connection_manager();

                auto player = connection_manager.get_player_for_connection(shared_from_this());
                auto room = player->room;

                if (room) {
                    // 룸에서 플레이어 제거
                    bool removed = room->remove_player(player);

                    if (removed) {
                        // 변경된 룸 정보를 브로드캐스트
                        json room_info;
                        room_info["room_id"] = room->id;
                        room_info["players"] = json::array();

                        for (const auto& p : room->get_players()) {
                            room_info["players"].push_back({
                                {"id", p->id},
                                {"name", p->name}
                            });
                        }

                        std::string response = create_response_string(RequestType::OUT, room_info.dump());
                        room->broadcast_message(response);
                    } else {
                        std::cout << "[WARNING] Player is already removed from the room.\n";
                    }
                }

            });

            break;
        }
        // 기타
        default:
            std::cout << "Unknown request type from header.\n";
            break;
        }

    } catch (const std::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
        enqueue_callback_(Event{EventType::ERROR, shared_from_this(), {}});
    }
}

void Connection::onWrite(const std::vector<char>& data) {
    std::cout << "Data written to client." << std::endl;
    // 미구현
}

void Connection::onClose() {
    std::cout << "Connection closed." << std::endl;
    if (!socket_.is_open()) {
        std::cout << "[INFO] Socket already closed.\n";
        return;
    }

    Event ev;
    ev.type = EventType::READ;
    ev.connection = shared_from_this();
    ev.request_type = RequestType::OUT;

    enqueue_callback_(ev); // OUT 이벤트 등록록

    // 소켓 닫기
    boost::system::error_code ec;
    socket_.close(ec); // 소켓 강제 종료
    if (ec) {
        std::cerr << "Failed to close socket: " << ec.message() << std::endl;
    }
}

void Connection::onError(const std::vector<char>& data) {
    std::string error_message(data.begin(), data.end());
    std::cerr << "Error: " << error_message << std::endl;

    // Close 이벤트 큐에 등록
    enqueue_callback_(Event{EventType::CLOSE, shared_from_this(), {}});
}

