#include "connection.hpp"

Connection::Connection(tcp::socket socket)
    : socket_(std::move(socket)) {}

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
                    enqueue_callback_(Event{EventType::ERROR, self, std::vector<char>(e.what(), e.what() + std::strlen(e.what()))});
                }
            } else {
                std::string error_message = ec ? ec.message() : "Unknown error while reading header.";
                enqueue_callback_(Event{EventType::ERROR, self, std::vector<char>(error_message.begin(), error_message.end())});
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
                    // 모든 데이터를 읽었다면, 이벤트 큐에 등록
                    enqueue_callback_(Event{EventType::READ, self, *body_buffer});
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

void Connection::onRead(const std::vector<char>& data) {
    std::string message(data.begin(), data.end());
    std::cout << "Received data: " << message << std::endl;
    // 미구현
}

void Connection::onWrite(const std::vector<char>& data) {
    std::cout << "Data written to client." << std::endl;
    // 미구현
}

void Connection::onClose() {
    std::cout << "Connection closed." << std::endl;
    // 미구현
}

void Connection::onError(const std::vector<char>& data) {
    std::string error_message(data.begin(), data.end());
    std::cerr << "Error: " << error_message << std::endl;
    // 미구현
}