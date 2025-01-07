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

    void start();

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
};

#endif // CONNECTION_HPP
