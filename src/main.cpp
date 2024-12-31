#include "reactor.hpp"
#include "thread_pool.hpp"
#include "server.hpp"

int main() {
    std::cout << "[main] Starting server..." << std::endl;

    ThreadPool thread_pool;
    Reactor reactor(12345, thread_pool);

    // 서버 싱글톤 접근
    Server& server = Server::getInstance();
    server.initialize_game();

    std::cout << "[main] Running reactor..." << std::endl;
    reactor.run();

    return 0;
}

