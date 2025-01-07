#include "connection.hpp"
#include "reactor.hpp"
#include <boost/asio.hpp>
#include <iostream>

#define PER_BYTE 8

Connection::Connection(tcp::socket socket)
    : socket_(std::move(socket))
{
}

void Connection::start() {
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
                        // 잘못된 헤더 -> ERROR 이벤트(임시로 CLOSE로 처리)
                        Event ev;
                        ev.main_type = MainEventType::NETWORK;
                        ev.sub_type  = (uint16_t)NetworkSubType::CLOSE;
                        ev.connection = self;
                        std::string msg = "Invalid Header";
                        ev.data.assign(msg.begin(), msg.end());
                        Reactor::get_instance().enqueue_event(ev);
                    }
                } catch (const std::exception& e) {
                    Event ev;
                    ev.main_type = MainEventType::NETWORK;
                    ev.sub_type  = (uint16_t)NetworkSubType::CLOSE; 
                    ev.connection = self;
                    std::string msg(e.what());
                    ev.data.assign(msg.begin(), msg.end());
                    Reactor::get_instance().enqueue_event(ev);
                }
            }
            else if (ec == boost::asio::error::eof || ec == boost::asio::error::connection_reset) {
                // 연결 종료
                Event ev;
                ev.main_type = MainEventType::NETWORK;
                ev.sub_type  = (uint16_t)NetworkSubType::CLOSE;
                ev.connection = self;
                Reactor::get_instance().enqueue_event(ev);
            }
            else {
                // 기타 에러
                std::string msg = ec ? ec.message() : "Unknown error";
                Event ev;
                ev.main_type = MainEventType::NETWORK;
                ev.sub_type  = (uint16_t)NetworkSubType::CLOSE; 
                ev.connection = self;
                ev.data.assign(msg.begin(), msg.end());
                Reactor::get_instance().enqueue_event(ev);
            }
        }
    );
}

bool Connection::is_header(const std::vector<char>& buffer) {
    // 1) 8 byte 확인
    if (buffer.size() < PER_BYTE) {
        return false;
    }

    // 2) 헤더를 파싱
    Header header;
    std::memcpy(&header, buffer.data(), sizeof(Header));

    // 3) main_type에 따라 sub_type 범위 확인
    switch (header.main_type) {
    case MainEventType::NETWORK:
        return (header.sub_type >= 101 && header.sub_type <= 199);

    case MainEventType::GAME:
        return (header.sub_type >= 201 && header.sub_type <= 299);

    default:
        return false; // 알 수 없는 main_type
    }
}

Header Connection::parse_header(const std::vector<char>& buffer) {
    if (buffer.size() < PER_BYTE) {
        throw std::invalid_argument("Invalid buffer size for header");
    }

    Header header;
    std::memcpy(&header, buffer.data(), sizeof(Header));
    return header;
}

void Connection::read_body(const Header& header) {
    auto self = shared_from_this();

    if (header.body_length > 10 * 1024 * 1024) {
        // 너무 큰 바디 -> 에러
        Event ev;
        ev.main_type = MainEventType::NETWORK;
        ev.sub_type  = (uint16_t)NetworkSubType::CLOSE;
        ev.connection= self;
        std::string msg = "Body too big";
        ev.data.assign(msg.begin(), msg.end());
        Reactor::get_instance().enqueue_event(ev);
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

                    // 이벤트 큐에 등록
                    Event ev;
                    ev.main_type = header.main_type;
                    ev.sub_type  = header.sub_type;
                    ev.connection= self;
                    ev.data      = std::move(actual_data);

                    Reactor::get_instance().enqueue_event(ev);

                    // 다음 요청 대비
                    async_read();
                }
            } else if (ec == boost::asio::error::eof || ec == boost::asio::error::connection_reset) {
                // 연결 종료
                Event ev;
                ev.main_type = MainEventType::NETWORK;
                ev.sub_type  = (uint16_t)NetworkSubType::CLOSE;
                ev.connection= self;
                Reactor::get_instance().enqueue_event(ev);
            } else {
                // 읽기 에러
                std::string msg = ec ? ec.message() : "Unknown error";
                Event ev;
                ev.main_type = MainEventType::NETWORK;
                ev.sub_type  = (uint16_t)NetworkSubType::CLOSE;
                ev.connection= self;
                ev.data.assign(msg.begin(), msg.end());
                Reactor::get_instance().enqueue_event(ev);
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
                // 필요 없음. - WRITE는 편리를 위해 바로 쓰도록 하는게 맞는것으로 보임.
            } else {
                // 쓰기 에러 -> CLOSE로 처리
                Event ev;
                ev.main_type = MainEventType::NETWORK;
                ev.sub_type  = (uint16_t)NetworkSubType::CLOSE;
                ev.connection= self;
                std::string msg = ec.message();
                ev.data.assign(msg.begin(), msg.end());
                Reactor::get_instance().enqueue_event(ev);
            }
        }
    );
}
