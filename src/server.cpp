#include "server.hpp"
#include <iostream>

// 싱글톤 인스턴스 반환
Server& Server::getInstance() {
    static Server instance;
    return instance;
}

// 생성자
Server::Server() {}

// 소멸자
Server::~Server() {}

// 게임 초기화
void Server::initialize_game() {
    add_new_room(); // 첫 번째 룸 생성
}

void Server::add_new_room() {
    auto new_room = std::make_shared<Room>(rooms.size());
    rooms.push_back(new_room);
    current_room = new_room;

    // 룸에 타이머 설정
    current_room->start_timer([this](int room_id) {
        this->on_room_timer_expired(room_id);
    });
}

void Server::on_room_timer_expired(int room_id) {
    std::lock_guard<std::mutex> lock(room_mutex);
    std::cout << "Room " << room_id << " timer expired." << std::endl;

    // 타이머가 만료된 룸에 대해 필요한 작업 수행
    // 1. 새로운 룸 생성
    if (current_room->id == room_id) {
        add_new_room();
    }
}

// 플레이어를 현재 활성화된 룸에 추가
void Server::add_player_to_room(std::shared_ptr<Player> player) {
    std::unique_lock<std::mutex> lock(room_mutex);

    if (!current_room->add_player(player)) {
        // 현재 룸이 가득 찬 경우 새 룸 생성
        add_new_room();
        current_room->add_player(player);
    }
}

std::shared_ptr<Room> Server::get_current_room() const {
    return current_room;
}
