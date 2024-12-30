#include "reactor.hpp"
#include "thread_pool.hpp"
#include "server.hpp"

int main() {
    ThreadPool thread_pool;
    Reactor reactor(12345, thread_pool); // 포트 번호 12345

    // 서버 시작
    reactor.run();

    // 싱글톤 인스턴스 접근
    Server& server = Server::getInstance();
    server.initialize_game();

    return 0;
}
