// reactor.hpp

#ifndef REACTOR_HPP
#define REACTOR_HPP

#include <boost/asio.hpp>
#include <memory>
#include <queue>
#include <unordered_map>
#include <functional>
#include "event.hpp"
#include "thread_pool.hpp"

using boost::asio::ip::tcp;

class Reactor {
public:
    explicit Reactor(unsigned short port, ThreadPool& thread_pool);

    void run();
    void enqueue_event(const Event& event);

private:
    void start_accept();
    void handle_accept(std::shared_ptr<tcp::socket> socket);
    void event_loop();

    boost::asio::io_context io_context_;
    tcp::acceptor acceptor_;
    std::queue<Event> event_queue_;
    std::unordered_map<EventType, std::function<void(std::shared_ptr<Connection>, const std::vector<char>&)>> handlers_;
    bool is_processing_ = false; // 이벤트 루프 실행 여부 플래그
    ThreadPool& thread_pool_;
};

#endif // REACTOR_HPP
