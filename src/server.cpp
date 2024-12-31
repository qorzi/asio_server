#include "server.hpp"
#include <iostream>

// 싱글톤 인스턴스 반환
Server& Server::getInstance() {
    static Server instance;
    std::cout << "[Server::getInstance] Singleton instance created or accessed." << std::endl;
    return instance;
}

// 생성자
Server::Server() {
    std::cout << "[Server::Server] Server constructor called." << std::endl;
}

// 소멸자
Server::~Server() {
    std::cout << "[Server::~Server] Server destructor called." << std::endl;
}

// 게임 초기화
void Server::initialize_game() {
    std::cout << "[Server::initialize_game] Initializing game and creating first room." << std::endl;
    add_new_room(); // 첫 번째 룸 생성
}

void Server::add_new_room() {
    std::cout << "[Server::add_new_room] Adding a new room. Total rooms: " << rooms.size() + 1 << std::endl;
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
    std::cout << "Room " << room_id << " expired." << std::endl;

    // 타이머가 만료된 룸에 대해 필요한 작업 수행
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

void Server::shutdown() {
    std::lock_guard<std::mutex> lock(room_mutex);

    // 모든 룸의 타이머 정지
    for (const auto& room : rooms) {
        room->stop_timer();
    }

    std::cout << "[Server::shutdown] All rooms and connections are stopped." << std::endl;
}

std::shared_ptr<Room> Server::get_current_room() const {
    return current_room;
}

ConnectionManager& Server::get_connection_manager() {
    return connection_manager_;
}