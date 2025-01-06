#include "server.hpp"
#include <iostream>

Server::Server() {
    std::cout << "[Server::Server] Constructor\n";
}

Server::~Server() {
    shutdown(); // 혹시 남아있는 룸의 타이머 정리
    std::cout << "[Server::~Server] Destructor\n";
}

void Server::initialize_server() {
    std::cout << "[Server::initialize_server] \n";
    // 현재 필요 없음.
}

void Server::shutdown() {
    std::lock_guard<std::mutex> lock(room_mutex_);
    // 모든 룸 타이머 정지
    for (const auto& room : rooms_) {
        room->stop_timer();
    }
    std::cout << "[Server::shutdown] All rooms closed.\n";
}

void Server::create_room() {
    std::lock_guard<std::mutex> lock(room_mutex_);
    auto new_room = std::make_shared<Room>(rooms_.size());
    rooms_.push_back(new_room);
    current_room_ = new_room;

    // 룸 타이머 콜백 설정
    current_room_->start_timer(
        [this](int room_id, bool is_start){
            this->on_room_timer_expired(room_id, is_start);
        }
    );
    std::cout << "[Server::create_room] Created room id=" << current_room_->id << "\n";
}

void Server::delete_room(int room_id) {
    std::lock_guard<std::mutex> lock(room_mutex_);
    auto it = std::find_if(rooms_.begin(), rooms_.end(),
        [room_id](const std::shared_ptr<Room>& r){ return r->id == room_id; });
    if (it != rooms_.end()) {
        rooms_.erase(it);
        std::cout << "[Server::delete_room] Removed room id=" << room_id << "\n";
    }
}

void Server::on_room_timer_expired(int room_id, bool is_start) {
    std::lock_guard<std::mutex> lock(room_mutex_);
    std::cout << "Room " << room_id << " expired." << std::endl;

    // 타이머가 만료된 룸에 대해 포인터 제거
    if (current_room_->id == room_id) {
        current_room_ = nullptr;
    }

    if (is_start) {
        // 게임 시작, 따로 처리할 내용 없음. - 게임의 관리 주체는 룸 객체임.
    } else {
        // 방 제거
        delete_room(room_id);
    }
}

void Server::add_player_to_room(std::shared_ptr<Player> player) {
    std::lock_guard<std::mutex> lock(room_mutex_);

    if (!current_room_) {
        create_room();
    }
    if (!current_room_->add_player(player)) {
        // current_room이 full인 경우
        create_room();
        current_room_->add_player(player);
    }
}

void Server::remove_player_to_room(std::shared_ptr<Player> player) {
    if (auto room_ptr = player->room.lock()) {
        room_ptr->remove_player(player);
    } else {
        std::cerr << "[WARNING] remove_player_to_room: room already expired?\n";
    }
}

std::shared_ptr<Room> Server::get_current_room() const {
    return current_room_;
}

ConnectionManager& Server::get_connection_manager() {
    return connection_manager_;
}