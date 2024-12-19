#ifndef REACTOR_HPP
#define REACTOR_HPP

#include <boost/asio.hpp>
#include <memory>

using boost::asio::ip::tcp;

class Reactor {
public:
    explicit Reactor(unsigned short port);
    void run();

private:
    void start_accept();
    void handle_accept(std::shared_ptr<tcp::socket> socket);

    boost::asio::io_context io_context_;
    tcp::acceptor acceptor_;
};

#endif // REACTOR_HPP