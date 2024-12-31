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
    read_header(); // 먼저 헤더 읽기 시작
}

void Connection::read_header() {
    auto header_buffer = std::make_shared<std::vector<char>>(sizeof(Header));
    auto self = shared_from_this();

    boost::asio::async_read(socket_, boost::asio::buffer(*header_buffer),
        [this, self, header_buffer](const boost::system::error_code& ec, std::size_t bytes_transferred) {
            if (!ec && bytes_transferred == sizeof(Header)) {
                try {
                    // 헤더 파싱
                    Header header = parse_header(*header_buffer);

                    // 바디 읽기 시작
                    read_body(header);
                } catch (const std::exception& e) {
                    // 예외 발생 시 이벤트로 처리
                    enqueue_callback_(Event{
                        EventType::ERROR,           // 에러 이벤트 타입
                        self,                       // 연결된 객체 (weak_ptr로 전달)
                        RequestType::UNKNOWN,       // 에러 시 RequestType은 UNKNOWN
                        std::vector<char>(e.what(), e.what() + std::strlen(e.what()))
                    });
                }
            } else {
                // 헤더 읽기 실패 시 이벤트로 처리
                std::string error_message = ec ? ec.message() : "Unknown error while reading header.";
                enqueue_callback_(Event{
                    EventType::ERROR,           // 에러 이벤트 타입
                    self,                       // 연결된 객체 (weak_ptr로 전달)
                    RequestType::UNKNOWN,       // 에러 시 RequestType은 UNKNOWN
                    std::vector<char>(error_message.begin(), error_message.end())
                });
            }
        });
}

Header Connection::parse_header(const std::vector<char>& buffer) {
    if (buffer.size() < sizeof(Header)) {
        throw std::invalid_argument("Buffer size is too small to parse Header.");
    }

    Header header;
    std::memcpy(&header, buffer.data(), sizeof(Header));
    return header;
}

void Connection::read_body(const Header& header) {
    auto body_buffer = std::make_shared<std::vector<char>>(header.body_length);
    continue_body_read(body_buffer, header, 0);
}

void Connection::continue_body_read(std::shared_ptr<std::vector<char>> body_buffer, const Header& header, std::size_t bytes_read) {
    auto self = shared_from_this();

    boost::asio::async_read(socket_, boost::asio::buffer(body_buffer->data() + bytes_read, header.body_length - bytes_read),
        [this, self, body_buffer, header, bytes_read](const boost::system::error_code& ec, std::size_t bytes_transferred) {
            if (!ec) {
                std::size_t total_bytes_read = bytes_read + bytes_transferred;
                if (total_bytes_read < header.body_length) {
                    // 다 읽지 못했다면, 계속 읽음
                    continue_body_read(body_buffer, header, total_bytes_read);
                } else {
                    // 모든 데이터를 읽었다면, READ 이벤트 등록
                    Event ev;
                    ev.type = EventType::READ;
                    ev.connection = self;
                    ev.request_type = header.type;
                    ev.data = *body_buffer;

                    enqueue_callback_(ev);

                    async_read(); // 다음 수신 대기
                }
            } else {
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

void Connection::onRead(const std::vector<char>& data, RequestType type) { 
    try {
        std::string message(data.begin(), data.end());
        std::cout << "Received data: " << message << std::endl;

        // json 데이터 가정(필수수)
        // {
        //     "player_id": "12345",
        //     "player_name": "Alice"
        // }

        // JSON 파싱
        json parsed_data = json::parse(data);
        // 클라이언트에서 보낸 데이터에서 플레이어 정보 추출
        std::string player_id = parsed_data["player_id"].get<std::string>();
        std::string player_name = parsed_data["player_name"].get<std::string>();

        // 요청 타입에 따라 분기 처리
        switch (type) {
        case RequestType::IN:
        {
            // 서버 싱글톤 인스턴스 가져오기
            Server& server = Server::getInstance();
            
            // 작업을 람다 함수로 정의
            thread_pool_.enqueue_task([player_id, player_name, &server]() {
                // 새로운 플레이어 객체 생성
                auto player = std::make_shared<Player>(player_id, player_name);

                // 룸에 플레이어 추가
                server.add_player_to_room(player);

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

                    room->broadcast_message(room_info.dump()); // 방 전체에 룸 정보 브로드캐스트
                }

            });

            break;
        }
        case RequestType::OUT:
        {
            // OUT 로직
            break;
        }
        // 기타타
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

