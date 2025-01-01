#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <boost/asio.hpp>
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <functional>
#include "event.hpp"
#include "header.hpp"

using boost::asio::ip::tcp;

class Connection : public std::enable_shared_from_this<Connection> {
public:
    explicit Connection(tcp::socket socket);

    // 리액터로 이벤트를 알릴 때 사용할 콜백(이벤트 enqueue 함수)
    void start_monitoring(std::function<void(const Event&)> enqueue_callback);

    void async_write(const std::string& response);

    tcp::socket& get_socket() {
        return socket_;
    }

private:
    void async_read();
    void read_chunk();
    bool is_header(const std::vector<char>& buffer);
    static Header parse_header(const std::vector<char>& buffer);
    void read_body(const Header& header);
    void read_body_chunk(std::shared_ptr<std::vector<char>> body_buffer, 
                         const Header& header, 
                         std::size_t bytes_read);

    tcp::socket socket_;
    // 이벤트 큐에 등록하기 위한 콜백
    std::function<void(const Event&)> enqueue_callback_;
};

#endif // CONNECTION_HPP
