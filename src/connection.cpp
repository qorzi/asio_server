#include "connection.hpp"
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
                    Header header = parse_header(*header_buffer);
                    read_body(header); // 헤더가 성공적으로 읽힌 경우, 바디 읽기 시작
                } catch (const std::exception& e) {
                    enqueue_callback_(Event{
                        EventType::ERROR,
                        self,
                        std::vector<char>(e.what(), e.what() + std::strlen(e.what()))
                    });
                }
            } else {
                std::string error_message = ec ? ec.message() : "Unknown error while reading header.";
                enqueue_callback_(Event{
                    EventType::ERROR,
                    self,
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
                    ev.event_type = EventType::READ;
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
    std::string message(data.begin(), data.end());
    std::cout << "Received data: " << message << std::endl;
    
    try {
        // JSON 파싱
        json parsed_data = json::parse(data);

        // std::string body  = parsed_data["something"].get<std::string>();

        // 요청 타입에 따라 분기 처리
        switch (type) {
        case RequestType::IN:
        {
            // 스레드 풀에 작업 등록 (예시: “게임 접속 처리”)
            std::string task_info = "게임에 접속 요청 (body: " + body + ")";
            thread_pool_.enqueue_task(task_info);

            // 필요하면 응답 전송
            // async_write(...);

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

