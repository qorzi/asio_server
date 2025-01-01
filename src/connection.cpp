#include "connection.hpp"
#include "server.hpp"
#include "event_handler.hpp"

Connection::Connection(tcp::socket socket, ThreadPool& thread_pool)
    : socket_(std::move(socket))
    , thread_pool_(thread_pool) {}

void Connection::start_monitoring(std::function<void(const Event&)> enqueue_callback) {
    enqueue_callback_ = std::move(enqueue_callback);
    async_read(); // REQUEST 이벤트 감지 시작
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

    return (type == RequestType::JOIN || type == RequestType::LEFT) && body_length > 0;
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
                    ev.type = EventType::REQUEST;
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

void Connection::onEvent(const std::vector<char>& data, RequestType type) { 
    try {
        auto self = shared_from_this();
        std::string message(data.begin(), data.end());
        std::cout << "Received data: " << message << std::endl;

        // 요청 타입에 따라 분기 처리
        switch (type) {
        case RequestType::JOIN:
        {            
            thread_pool_.enqueue_task([data, self]() {
                EventHandler::handle_in_event(data, self);
            });

            break;
        }
        case RequestType::LEFT:
        {
            thread_pool_.enqueue_task([data, self]() {
                EventHandler::handle_out_event(data, self);
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
    ev.type = EventType::REQUEST;
    ev.connection = shared_from_this();
    ev.request_type = RequestType::LEFT;

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

