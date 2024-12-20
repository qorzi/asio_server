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
    connection->start_monitoring([this](const Event& event) {
        this->enqueue_event(event); // Reactor의 이벤트 큐에 등록
    });
}

void Reactor::event_loop() {
    is_processing_ = true;
    while (!event_queue_.empty()) {
        Event event = event_queue_.front();
        event_queue_.pop();

        // 객체 상태 확인
        if (auto connection = event.connection.lock()) {
            switch (event.type) {
                case EventType::READ:
                    connection->onRead(event.data);
                    break;
                case EventType::WRITE:
                    connection->onWrite(event.data);
                    break;
                case EventType::CLOSE:
                    connection->onClose();
                    break;
                case EventType::ERROR:
                    connection->onError(event.data);
                    break;
            }
        } else {
            std::cerr << "Connection object is no longer valid." << std::endl;
        }
    }
    is_processing_ = false;
}


void Reactor::enqueue_event(const Event& event) {
    event_queue_.push(event);
    if (!is_processing_) {
        event_loop(); // 큐 처리 시작
    }
}
