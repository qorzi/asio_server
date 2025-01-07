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
#include "network_event_handler.hpp"
#include "game_event_handler.hpp"

using boost::asio::ip::tcp;

class Reactor {
public:
    static Reactor& get_instance() {
        if (!instance_) {
            throw std::runtime_error("Reactor instance is not initialized. Call initialize_instance() first.");
        }
        return *instance_;
    }

    static void initialize_instance(boost::asio::io_context& ioc, unsigned short port, ThreadPool& thread_pool, GameManager& gm) {
        if (!instance_) {
            instance_ = std::make_unique<Reactor>(ioc, port, thread_pool, gm);
        }
    }

    void run();
    void enqueue_event(const Event& event);

private:
    Reactor(boost::asio::io_context& ioc, unsigned short port, ThreadPool& thread_pool, GameManager& gm);

    static std::unique_ptr<Reactor> instance_;

    // 복사 및 이동 생성자 삭제
    Reactor(const Reactor&) = delete;
    Reactor& operator=(const Reactor&) = delete;
    Reactor(Reactor&&) = delete;
    Reactor& operator=(Reactor&&) = delete;

    void start_accept();
    void handle_accept(std::shared_ptr<tcp::socket> socket);
    void event_loop();

    boost::asio::io_context& ioc_;
    tcp::acceptor acceptor_;
    ThreadPool& thread_pool_;
    NetworkEventHandler network_handler_;
    GameEventHandler game_handler_;

    std::queue<Event> event_queue_;
    bool is_processing_ = false; // 이벤트 루프 실행 여부 플래그
};

#endif // REACTOR_HPP
