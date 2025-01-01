#include "reactor.hpp"
#include "connection.hpp"
#include "event_handler.hpp"  // 가령 기존에 쓰시던 static 함수 모음

Reactor::Reactor(unsigned short port, ThreadPool& thread_pool)
    : acceptor_(io_context_, tcp::endpoint(tcp::v4(), port))
    , thread_pool_(thread_pool) {}

void Reactor::run() {
    start_accept();
    io_context_.run();
}

void Reactor::start_accept() {
    auto socket = std::make_shared<tcp::socket>(io_context_);
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

        // Connection이 살아있는지 확인
        if (auto connection = event.connection.lock()) {
            switch (event.type) {
            case EventType::REQUEST:
            {
                // 예: join/left 분기 처리
                switch (event.request_type) {
                case RequestType::JOIN:
                    thread_pool_.enqueue_task([connection, event]() {
                        EventHandler::handle_in_event(event.data, connection);
                    });
                    break;

                case RequestType::LEFT:
                    thread_pool_.enqueue_task([connection, event]() {
                        EventHandler::handle_out_event(event.data, connection);
                    });
                    break;

                default:
                    // Unknown request type
                    std::cout << "Unknown request type.\n";
                    break;
                }
                break;
            }
            case EventType::WRITE:
            {
                std::cout << "Data successfully written to client.\n";
                break;
            }
            case EventType::CLOSE:
            {
                std::cout << "[INFO] Reactor: CLOSE event\n";
                thread_pool_.enqueue_task([connection, event]() {
                    EventHandler::handle_close_event(event.data, connection);
                });
                break;
            }
            case EventType::ERROR:
            {
                // 에러 처리
                std::string error_msg(event.data.begin(), event.data.end());
                std::cerr << "Error occurred: " << error_msg << std::endl;
                thread_pool_.enqueue_task([connection, event]() {
                    EventHandler::handle_close_event(event.data, connection);
                });
                break;
            }
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
        event_loop(); 
    }
}
