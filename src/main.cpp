#include "reactor.hpp"
#include "thread_pool.hpp"

int main() {
    ThreadPool thread_pool;
    Reactor reactor(12345, thread_pool); // 포트 번호 12345

    // 서버 시작
    reactor.run();

    return 0;
}
