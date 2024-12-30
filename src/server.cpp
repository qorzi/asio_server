#include "server.hpp"
#include <iostream>

// 싱글톤 인스턴스 반환
Server& Server::getInstance() {
    static Server instance;
    return instance;
}

// 생성자
Server::Server() : room_timer_active(false) {
    // 첫 번째 룸 생성
    current_room = std::make_shared<Room>(0);
    rooms.push_back(current_room);
}

// 소멸자
Server::~Server() {}

// 게임 초기화
void Server::initialize_game() {
    // 초기화 작업 (필요하면 추가 구현)
}

// 플레이어를 현재 활성화된 룸에 추가
void Server::add_player_to_room(std::shared_ptr<Player> player) {
    std::unique_lock<std::mutex> lock(room_mutex);

    if (!current_room->add_player(player)) {
        // 현재 룸이 가득 찬 경우 새 룸 생성
        current_room = std::make_shared<Room>(rooms.size());
        rooms.push_back(current_room);
        current_room->add_player(player);
    }

    if (current_room->get_players().size() == 1 && !room_timer_active.load()) {
        room_timer_active.store(true);
        lock.unlock(); // 타이머 시작 시 뮤텍스 해제
        start_room_timer();
    }
}

// 룸 타이머 시작
void Server::start_room_timer() {
    std::thread([this]() {
        std::this_thread::sleep_for(std::chrono::seconds(30));

        std::unique_lock<std::mutex> lock(room_mutex);
        if (current_room && current_room->get_players().size() < 30) {
            std::cout << "Room " << current_room->id << " is not full. Creating a new room." << std::endl;
            current_room = std::make_shared<Room>(rooms.size());
            rooms.push_back(current_room);
        }
        room_timer_active.store(false); // 타이머 종료
    }).detach();
}

// 브로드캐스팅 함수
void Server::broadcast_to_room(int room_id, const std::string& message) {
    std::unique_lock<std::mutex> lock(room_mutex);
    if (room_id >= 0 && room_id < rooms.size()) {
        auto& room = rooms[room_id];
        for (const auto& player : room->get_players()) {
            // 각 플레이어에게 메시지를 전송
            player->send_message(message);
        }
    } else {
        std::cerr << "Room ID " << room_id << " does not exist!" << std::endl;
    }
}
