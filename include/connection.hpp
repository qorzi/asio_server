#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <boost/asio.hpp>
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include "event.hpp"
#include "header.hpp"

using boost::asio::ip::tcp;

class Connection : public std::enable_shared_from_this<Connection> {
public:
    explicit Connection(tcp::socket socket);

    void onRead(const std::vector<char>& data);
    void onWrite(const std::vector<char>& data);
    void onClose();

    void start_monitoring(std::function<void(const Event&)> enqueue_callback);

private:
    void async_read();
    void read_header();
    static Header parse_header(const std::vector<char>& header);
    void read_body(const Header& header);
    void continue_body_read(std::shared_ptr<std::vector<char>> body_buffer, const Header& header, std::size_t bytes_read);
    void async_write(const std::string& response);

    tcp::socket socket_;
    std::function<void(const Event&)> enqueue_callback_;
};

#endif // CONNECTION_HPP