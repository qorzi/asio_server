#include "reactor.hpp"

int main() {
    Reactor reactor(12345);  // 포트 번호
    reactor.run();
    return 0;
}
