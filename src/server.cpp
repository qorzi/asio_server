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
    shutdown(); // 별도의 스레드에서 실행 중인 모든 타이머 종료
    std::cout << "[Server::~Server] Server destructor called." << std::endl;
}

// 서버버 초기화
void Server::initialize_server() {
    std::cout << "[Server::initialize_server] Initializing server." << std::endl;
}

void Server::create_room() {
    std::cout << "[Server::add_new_room] Adding a new room. Total rooms: " << rooms.size() + 1 << std::endl;
    auto new_room = std::make_shared<Room>(rooms.size());
    rooms.push_back(new_room);
    current_room = new_room;

    // 룸에 타이머 설정
    current_room->start_timer([this](int room_id, bool is_start) {
        this->on_room_timer_expired(room_id, is_start);
    });
}

void Server::delete_room(int room_id) {
    std::cout << "[Server::delete_room] Attempting to delete room with ID " << room_id << "." << std::endl;

    auto it = std::find_if(rooms.begin(), rooms.end(), [room_id](const std::shared_ptr<Room>& room) {
        return room->id == room_id;
    });

    if (it != rooms.end()) {
        rooms.erase(it);
        std::cout << "[Server::delete_room] Successfully removed room " << room_id << " from rooms vector." << std::endl;
    } else {
        std::cout << "[Server::delete_room] No room found with ID " << room_id << "." << std::endl;
    }
}

void Server::on_room_timer_expired(int room_id, bool is_start) {
    std::lock_guard<std::mutex> lock(room_mutex);
    std::cout << "Room " << room_id << " expired." << std::endl;

    // 타이머가 만료된 룸에 대해 포인터 제거
    if (current_room->id == room_id) {
        current_room = nullptr;
    }

    if (is_start) {
        // 게임 시작, 따로 처리할 내용 없음. - 게임의 관리 주체는 룸 객체임.
    } else {
        // 방 제거
        delete_room(room_id);
    }
}

// 플레이어를 현재 활성화된 룸에 추가
void Server::add_player_to_room(std::shared_ptr<Player> player) {
    std::unique_lock<std::mutex> lock(room_mutex);

    // current_room이 없는 경우 새 룸 생성
    if (!current_room) {
        create_room(); // 첫 번째 룸 생성
    }

    // current_room에 플레이어 추가 시도
    if (!current_room->add_player(player)) {
        // 현재 룸이 가득 찬 경우 새 룸 생성
        create_room();
        current_room->add_player(player);
    }
}

// 플레이어를 룸에서 제거
void Server::remove_player_to_room(std::shared_ptr<Player> player) {
    if (auto room_ptr = player->room.lock()) {
        room_ptr->remove_player(player);
    } else {
        // room_ptr이 이미 소멸되었다면(weak_ptr expired)
        std::cerr << "[WARNING] Cannot remove player from room: room has expired.\n";
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