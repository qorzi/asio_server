#include "reactor.hpp"
#include "connection.hpp"
#include "event_handler.hpp"

Reactor::Reactor(boost::asio::io_context& ioc, unsigned short port, ThreadPool& thread_pool)
    : ioc_(ioc)
    , acceptor_(ioc, tcp::endpoint(tcp::v4(), port))
    , thread_pool_(thread_pool)
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
        // 다음 연결도 대기
        start_accept();
    });
}

void Reactor::handle_accept(std::shared_ptr<tcp::socket> socket) {
    std::cout << "[Reactor] New connection accepted.\n";
    auto connection = std::make_shared<Connection>(std::move(*socket));
    // 이벤트가 발생하면 Reactor에게 알림
    connection->start_monitoring([this](const Event& event) {
        enqueue_event(event);
    });
}

// 이벤트 큐에서 꺼내어 처리
void Reactor::event_loop() {
    is_processing_ = true;

    while (!event_queue_.empty()) {
        Event event = event_queue_.front();
        event_queue_.pop();

        // 각 EventType별로 작업 스케줄링
        switch (event.type) {
        case EventType::REQUEST:
        {
            // request_type에 따라 처리
            switch (event.request_type) {
            case RequestType::JOIN:
                thread_pool_.enqueue_task([event]() {
                    // ThreadPool 안의 worker_thread()에서 try-catch 처리
                    EventHandler::handle_join_event(event);
                });
                break;
            case RequestType::LEFT:
                thread_pool_.enqueue_task([event]() {
                    EventHandler::handle_left_event(event);
                });
                break;
            // ... 다른 RequestType: SET, START, PLAY, END, etc.
            default:
                std::cout << "[WARNING] Unknown request type.\n";
                break;
            }
            break;
        }
        case EventType::WRITE:
        {
            std::cout << "[INFO] Reactor: WRITE event\n";
            break;
        }
        case EventType::CLOSE:
        {
            std::cout << "[INFO] Reactor: CLOSE event\n";
            thread_pool_.enqueue_task([event]() {
                EventHandler::handle_close_event(event);
            });
            break;
        }
        case EventType::ERROR:
        {
            std::string err_msg(event.data.begin(), event.data.end());
            std::cerr << "[ERROR] Reactor got ERROR event: " << err_msg << std::endl;

            // 대부분 ERROR 이벤트 = 소켓 닫기 or cleanup
            thread_pool_.enqueue_task([event]() {
                EventHandler::handle_close_event(event);
            });
            break;
        }
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
