#ifndef GAME_SERVER_APP_HPP
#define GAME_SERVER_APP_HPP

#include <memory>
#include "game_manager.hpp"
#include "reactor.hpp"
#include "thread_pool.hpp"

/**
 * 상위(Orchestrator) 역할을 하는 클래스.
 * - Reactor, ThreadPool, Server 등을 생성하고 소유
 * - 서버 구동(start), 정지(stop) 라이프사이클 제어
 */
class GameServerApp {
public:
    // 예시로 포트 번호를 인자로 받도록 구성
    explicit GameServerApp(unsigned short port = 12345);
    ~GameServerApp();

    // 서버 시작(초기화 + 리액터 run 등)
    void start();

    // 서버 정지
    void stop();

private:
    // 서버 실행 여부
    bool running_ = false;

    // 주요 구성 요소
    boost::asio::io_context io_context_;
    std::unique_ptr<ThreadPool> thread_pool_;
    std::unique_ptr<GameManager> game_manager_;
};

#endif // GAME_SERVER_APP_HPP
