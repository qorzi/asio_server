#include "server.hpp"
#include <iostream>

Server::Server() {
    // 첫 번째 룸 생성
    current_room = std::make_shared<Room>(0);
    rooms.push_back(current_room);
}

void Server::initialize_game() {
    // 초기화 작업 (필요하면 구현 추가)
}

void Server::add_player_to_room(std::shared_ptr<Player> player) {
    std::unique_lock<std::mutex> lock(room_mutex);

    // 현재 룸에 추가 시도
    if (!current_room->add_player(player)) {
        // 현재 룸이 가득 찬 경우 새 룸 생성
        current_room = std::make_shared<Room>(rooms.size());
        rooms.push_back(current_room);
        current_room->add_player(player);
    }

    // 첫 번째 플레이어가 추가된 경우에만 타이머 시작
    if (current_room->players.size() == 1) {
        start_room_timer();
    }
}

void Server::start_room_timer() {
    std::thread([this]() {
        std::this_thread::sleep_for(std::chrono::seconds(30));
        std::unique_lock<std::mutex> lock(room_mutex);
        if (current_room && current_room->players.size() < 30) {
            // 30초가 지난 경우 새로운 룸 생성
            current_room = std::make_shared<Room>(rooms.size());
            rooms.push_back(current_room);
        }
    }).detach();
}