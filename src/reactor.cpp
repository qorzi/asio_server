#include "reactor.hpp"
#include "connection.hpp"

std::unique_ptr<Reactor> Reactor::instance_ = nullptr;

Reactor::Reactor(boost::asio::io_context& ioc, unsigned short port, ThreadPool& thread_pool, GameManager& gm)
    : acceptor_(ioc, tcp::endpoint(tcp::v4(), port))
    , thread_pool_(thread_pool)
    , ioc_(ioc)
    , network_handler_(gm)
    , game_handler_(gm, ioc)
{
    std::cout << "[Reactor] Constructor - port:" << port << "\n";
}

void Reactor::run() {
    start_accept();
}

void Reactor::start_accept() {
    auto socket = std::make_shared<tcp::socket>(ioc_);
    acceptor_.async_accept(*socket, [this, socket](const boost::system::error_code& ec) {
        if (!ec) {
            handle_accept(socket);
        }
        start_accept();
    });
}

void Reactor::handle_accept(std::shared_ptr<tcp::socket> socket) {
    auto connection = std::make_shared<Connection>(std::move(*socket));
    // 이벤트가 발생하면 Reactor에게 알림
    connection->start();
}

// 이벤트 큐에서 꺼내어 처리
void Reactor::event_loop() {
    is_processing_ = true;

    while (!event_queue_.empty()) {
        Event event = event_queue_.front();
        event_queue_.pop();

        // 각 EventType별로 작업 스케줄링
        switch (event.main_type) {
        case MainEventType::NETWORK:
        {
            thread_pool_.enqueue_task([this, event]() {
                // ThreadPool 안의 worker_thread()에서 try-catch 처리
                network_handler_.handle_event(event);
            });
            break;
        }
        case MainEventType::GAME:
        {
            thread_pool_.enqueue_task([this, event]() {
                // ThreadPool 안의 worker_thread()에서 try-catch 처리
                game_handler_.handle_event(event);
            });
            break;
        }
        default:
            // unknown
            break;
        }
    }

    is_processing_ = false;
}

void Reactor::enqueue_event(const Event& event) {
    event_queue_.push(event);
    if (!is_processing_) {
        event_loop();
    }
}
