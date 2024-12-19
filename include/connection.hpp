#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <boost/asio.hpp>
#include <memory>
#include <vector>
#include <string>
#include <iostream>

using boost::asio::ip::tcp;

class Connection : public std::enable_shared_from_this<Connection> {
public:
    explicit Connection(tcp::socket socket);
    void start();

private:
    void read_header();
    void read_body(uint32_t body_length);
    void process_and_enqueue(const std::string& body);
    void write_response(const std::string& response);

    tcp::socket socket_;
};

#endif // CONNECTION_HPP