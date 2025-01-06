#include "game_server_app.hpp"
#include "event_handler.hpp"
#include <iostream>

GameServerApp::GameServerApp(unsigned short port)
{
    std::cout << "[GameServerApp] Constructor\n";

    // 1) 스레드풀 생성 (기본 스레드 개수: std::thread::hardware_concurrency())
    thread_pool_ = std::make_unique<ThreadPool>();

    // 2) 리액터 생성 (포트 번호와 thread_pool 참조)
    reactor_ = std::make_unique<Reactor>(io_context_, port, *thread_pool_);

    // 3) 게임 매니저 생성
    game_manager_ = std::make_unique<GameManager>(io_context_);
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

    // 1) GameManager 초기화
    game_manager_->initialize();

    // 2) EventHandler에 GameManager 등록
    EventHandler::init(game_manager_.get());

    // 3) Reactor 시작: async_accept 등
    std::cout << "[GameServerApp] Starting Reactor...\n";
    reactor_->run();


    // 4) io_context.run()을 블로킹으로 실행(= 메인 스레드 사용)
    //    - 만약 여러 스레드에서 io_context를 돌리고 싶다면,
    //      thread_pool_->enqueue_task([this](){ io_context_.run(); });
    //      처럼 여러 번 실행하면 됩니다. 
    //    - 여기서는 단일 스레드 예시
    std::cout << "[GameServerApp] Running io_context...\n";
    io_context_.run();

    std::cout << "[GameServerApp] start() done.\n";
}

void GameServerApp::stop()
{
    if (!running_) {
        return;
    }
    running_ = false;

    // GameManager 정리
    game_manager_->shutdown();

    // io_context 정지
    io_context_.stop();

    std::cout << "[GameServerApp] stop() called.\n";
}
