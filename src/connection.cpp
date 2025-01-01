#include "connection.hpp"
#include <boost/asio.hpp>
#include <iostream>

#define PER_BYTE 8

Connection::Connection(tcp::socket socket)
    : socket_(std::move(socket))
{
}

void Connection::start_monitoring(std::function<void(const Event&)> enqueue_callback) {
    enqueue_callback_ = std::move(enqueue_callback);
    async_read(); // 읽기 시작
}

void Connection::async_read() {
    read_chunk(); // 8바이트씩 헤더 읽기
}

void Connection::read_chunk() {
    auto chunk_buffer = std::make_shared<std::vector<char>>(PER_BYTE);
    auto self = shared_from_this();

    boost::asio::async_read(
        socket_,
        boost::asio::buffer(*chunk_buffer),
        [this, self, chunk_buffer](const boost::system::error_code& ec, std::size_t bytes_transferred)
        {
            if (!ec && bytes_transferred == PER_BYTE) {
                // 헤더인지 판별
                try {
                    if (is_header(*chunk_buffer)) {
                        Header header = parse_header(*chunk_buffer);
                        read_body(header);
                    } else {
                        // 잘못된 헤더라면 에러 이벤트
                        enqueue_callback_(
                            Event{
                                EventType::ERROR,
                                self,
                                RequestType::UNKNOWN,
                                {'I','n','v','a','l','i','d','H','d','r'}
                            }
                        );
                    }
                } catch (const std::exception& e) {
                    enqueue_callback_(
                        Event{
                            EventType::ERROR,
                            self,
                            RequestType::UNKNOWN,
                            std::vector<char>(e.what(), e.what() + std::strlen(e.what()))
                        }
                    );
                }
            }
            else if (ec == boost::asio::error::eof || ec == boost::asio::error::connection_reset) {
                // 연결 종료
                enqueue_callback_(
                    Event{
                        EventType::CLOSE,
                        self,
                        RequestType::UNKNOWN,
                        {}
                    }
                );
            }
            else {
                // 기타 에러
                std::string msg = ec ? ec.message() : "Unknown error";
                enqueue_callback_(
                    Event{
                        EventType::ERROR,
                        self,
                        RequestType::UNKNOWN,
                        std::vector<char>(msg.begin(), msg.end())
                    }
                );
            }
        }
    );
}

bool Connection::is_header(const std::vector<char>& buffer) {
    if (buffer.size() < PER_BYTE) {
        return false;
    }
    // 예시 조건
    RequestType type = static_cast<RequestType>(buffer[0]);
    uint32_t body_length = 0;
    for (int i = 0; i < 4; ++i) {
        body_length |= (static_cast<uint32_t>(buffer[1 + i]) << (i * 8));
    }
    return (type == RequestType::JOIN || type == RequestType::LEFT) && (body_length > 0);
}

Header Connection::parse_header(const std::vector<char>& buffer) {
    if (buffer.size() < PER_BYTE) {
        throw std::invalid_argument("Invalid buffer size for header");
    }

    Header header;
    header.type = static_cast<RequestType>(buffer[0]);
    header.body_length = 0;
    for (int i = 0; i < 4; ++i) {
        header.body_length |= (static_cast<uint32_t>(buffer[1 + i]) << (i * 8));
    }
    return header;
}

void Connection::read_body(const Header& header) {
    auto self = shared_from_this();

    if (header.body_length > 10 * 1024 * 1024) {
        // 너무 큰 바디 -> 에러
        enqueue_callback_(
            Event{
                EventType::ERROR,
                self,
                RequestType::UNKNOWN,
                {'B','o','d','y','T','o','o','B','i','g'}
            }
        );
        return;
    }

    size_t padded_size = ((header.body_length + 7) / PER_BYTE) * PER_BYTE;
    auto body_buffer = std::make_shared<std::vector<char>>(padded_size);

    read_body_chunk(body_buffer, header, 0);
}

void Connection::read_body_chunk(std::shared_ptr<std::vector<char>> body_buffer,
                                 const Header& header,
                                 std::size_t bytes_read)
{
    auto self = shared_from_this();
    size_t remaining = body_buffer->size() - bytes_read;

    boost::asio::async_read(
        socket_,
        boost::asio::buffer(body_buffer->data() + bytes_read, remaining),
        [this, self, body_buffer, header, bytes_read]
        (const boost::system::error_code& ec, std::size_t bytes_transferred)
        {
            if (!ec) {
                std::size_t total = bytes_read + bytes_transferred;
                if (total < body_buffer->size()) {
                    // 계속 읽기
                    read_body_chunk(body_buffer, header, total);
                } else {
                    // 완전히 읽음
                    // 패딩 제거
                    std::vector<char> actual_data(
                        body_buffer->begin(),
                        body_buffer->begin() + header.body_length
                    );

                    // REQUEST 이벤트 큐에 등록
                    enqueue_callback_(
                        Event{
                            EventType::REQUEST,
                            self,
                            header.type,
                            actual_data
                        }
                    );

                    // 다음 요청 대비
                    async_read();
                }
            } else if (ec == boost::asio::error::eof || ec == boost::asio::error::connection_reset) {
                // 연결 종료
                enqueue_callback_(
                    Event{
                        EventType::CLOSE,
                        self,
                        RequestType::UNKNOWN,
                        {}
                    }
                );
            } else {
                // 읽기 에러
                std::string msg = ec ? ec.message() : "Unknown error";
                enqueue_callback_(
                    Event{
                        EventType::ERROR,
                        self,
                        RequestType::UNKNOWN,
                        std::vector<char>(msg.begin(), msg.end())
                    }
                );
            }
        }
    );
}

void Connection::async_write(const std::string& data) {
    auto self = shared_from_this();
    auto send_buf = std::make_shared<std::string>(data);

    boost::asio::async_write(
        socket_,
        boost::asio::buffer(*send_buf),
        [this, self, send_buf](const boost::system::error_code& ec, std::size_t bytes_written)
        {
            if (!ec) {
                // 쓰기 완료 -> WRITE 이벤트
                enqueue_callback_(
                    Event{
                        EventType::WRITE,
                        self,
                        RequestType::UNKNOWN,
                        std::vector<char>(send_buf->begin(), send_buf->end())
                    }
                );
            } else {
                // 쓰기 에러 -> CLOSE로 처리
                enqueue_callback_(
                    Event{
                        EventType::CLOSE,
                        self,
                        RequestType::UNKNOWN,
                        {}
                    }
                );
            }
        }
    );
}
