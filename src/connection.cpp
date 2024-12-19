#include "connection.hpp"
#include "parser.hpp"
#include "thread_pool.hpp"

Connection::Connection(tcp::socket socket)
    : socket_(std::move(socket)) {}

void Connection::start() {
    read_header();
}

void Connection::read_header() {
    auto header = std::make_shared<std::vector<char>>(5);
    auto self = shared_from_this();
    boost::asio::async_read(socket_, boost::asio::buffer(*header),
        [this, self, header](const boost::system::error_code& ec, std::size_t) {
            if (!ec) {
                Header parsed_header = Parser::parse_header(*header);
                read_body(parsed_header.body_length);
            } else {
                std::cerr << "Error reading header: " << ec.message() << std::endl;
            }
        });
}

void Connection::read_body(uint32_t body_length) {
    auto body = std::make_shared<std::vector<char>>(body_length);
    auto self = shared_from_this();
    boost::asio::async_read(socket_, boost::asio::buffer(*body),
        [this, self, body](const boost::system::error_code& ec, std::size_t) {
            if (!ec) {
                std::string body_data(body->begin(), body->end());
                process_and_enqueue(body_data);
            } else {
                std::cerr << "Error reading body: " << ec.message() << std::endl;
            }
        });
}

void Connection::process_and_enqueue(const std::string& body) {
    std::string task = Parser::process_body(body);
    ThreadPool thread_pool;
    thread_pool.enqueue_task(task);

    auto response = "Processed: " + body;
    write_response(response);
}

void Connection::write_response(const std::string& response) {
    auto response_buffer = std::make_shared<std::string>(response);
    auto self = shared_from_this();
    boost::asio::async_write(socket_, boost::asio::buffer(*response_buffer),
        [this, self, response_buffer](const boost::system::error_code& ec, std::size_t) {
            if (ec) {
                std::cerr << "Error writing response: " << ec.message() << std::endl;
            }
        });
}