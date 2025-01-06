#include <iostream>
#include "game_server_app.hpp"

// main.cpp
int main() {
    std::cout << "[main] Starting GameServerApp..." << std::endl;
    GameServerApp app;
    app.start();
    return 0;
}
