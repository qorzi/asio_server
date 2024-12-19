#include "reactor.hpp"
#include "connection.hpp"

Reactor::Reactor(unsigned short port)
    : acceptor_(io_context_, tcp::endpoint(tcp::v4(), port)) {}

void Reactor::run() {
    start_accept();
    io_context_.run();
}

void Reactor::start_accept() {
    auto socket = std::make_shared<tcp::socket>(io_context_); // client socket
    acceptor_.async_accept(*socket, [this, socket](const boost::system::error_code& ec) {
        if (!ec) {
            handle_accept(socket);
        }
        start_accept(); // 다음 연결 대기
    });
}

void Reactor::handle_accept(std::shared_ptr<tcp::socket> socket) {
    // 클라이언트 연결 생성 및 시작
    auto connection = std::make_shared<Connection>(std::move(*socket));
    connection->start();
}