#include "room.hpp"
#include "player.hpp"

#include <thread>
#include <iostream>

Room::Room(int id, boost::asio::io_context& ioc)
    : id_(id)
    , ioc_(ioc)
{
    std::cout << "[DEBUG][Room:" << id << "] Room constructor called.\n";
    // 초기화
    initialize_maps();
}

void Room::initialize_maps() {
    // 맵 생성
    auto mapA = std::make_shared<Map>("A", 300, 300);
    mapA->start_point = {1, 1};

    auto mapB = std::make_shared<Map>("B", 300, 300);
    mapB->start_point = {1, 1};

    auto mapC = std::make_shared<Map>("C", 300, 300);
    mapC->start_point = {1, 1};
    mapC->end_point = {299, 299};

    maps_ = {mapA, mapB, mapC};

    // 포탈 생성 및 연결
    portal_links_[mapA->generate_random_portal(mapB->name)] = mapB;
    portal_links_[mapB->generate_random_portal(mapC->name)] = mapC;
}

bool Room::add_player(std::shared_ptr<Player> player) {
    std::lock_guard<std::mutex> lock(mutex_); // 뮤텍스 잠금

    // 유저 추가
    player->room_ = shared_from_this();
    player->current_map_ = maps_[0]; // 첫 번째 맵으로 설정
    player->position_ = maps_[0]->start_point;
    players_.push_back(player);

    // 디버그 메시지
    std::cout << "[DEBUG][Room:" << id_ << "] Player " << player->id_
              << " added. Current player count: " << players_.size() << "\n";

    return true;
}

bool Room::remove_player(std::shared_ptr<Player> player) {
    std::lock_guard<std::mutex> lock(mutex_); // 뮤텍스 잠금
    auto it = std::find(players_.begin(), players_.end(), player);

    if (it != players_.end()) {
        players_.erase(it);
        std::cout << "[DEBUG][Room:" << id_ << "] Player " << player->id_
                  << " removed. Current player count: " << players_.size() << "\n";
        return true;
    }

    std::cout << "[DEBUG][Room:" << id_ << "] remove_player() failed: player not found.\n";
    return false;
}

const std::vector<std::shared_ptr<Player>>& Room::get_players() const {
    return players_;
}

void Room::broadcast_message(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_); // 플레이어 리스트 보호

    std::cout << "[DEBUG][Room:" << id_ << "] Broadcasting message to " 
              << players_.size() << " players.\n";

    for (const auto& player : players_) {
        player->send_message(message); // 메시지 전송
    }
}

void Room::update_game_state() {
    // 미구현
}
