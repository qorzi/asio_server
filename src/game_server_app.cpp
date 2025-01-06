#include "game_server_app.hpp"
#include <iostream>

GameServerApp::GameServerApp(unsigned short port)
{
    std::cout << "[GameServerApp] Constructor\n";

    // 1) 스레드풀 생성 (기본 스레드 개수: std::thread::hardware_concurrency())
    thread_pool_ = std::make_unique<ThreadPool>();

    // 2) 리액터 생성 (포트 번호와 thread_pool 참조)
    reactor_ = std::make_unique<Reactor>(port, *thread_pool_);

    // 3) 서버(게임 로직) 생성
    //    (기존에는 싱글톤이었으나, 이제는 일반 객체로)
    server_ = std::make_unique<Server>();
}

GameServerApp::~GameServerApp()
{
    stop(); // 안전하게 stop() 호출
    std::cout << "[GameServerApp] Destructor\n";
}

void GameServerApp::start()
{
    if (running_) {
        std::cout << "[GameServerApp::start] Already running.\n";
        return;
    }

    running_ = true;

    // 1) 서버 초기화(방, 플레이어 등 준비)
    server_->initialize_server();

    // 2) 리액터 시작
    //    Reactor::run()은 io_context_.run()을 호출하여 블로킹 될 수도 있음
    //    필요시 별도 스레드에서 돌리거나, 현 스레드에서 돌리거나 선택 가능
    std::cout << "[GameServerApp] Starting Reactor...\n";
    reactor_->run();

    std::cout << "[GameServerApp] start() done.\n";
}

void GameServerApp::stop()
{
    if (!running_) {
        return;
    }
    running_ = false;

    // 1) 서버 shutdown
    server_->shutdown();

    // 2) Reactor/ThreadPool 정리
    //    현재 미구현

    std::cout << "[GameServerApp] stop() called.\n";
}
