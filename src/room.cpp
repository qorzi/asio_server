#include "room.hpp"
#include "player.hpp"

Room::Room(int id) : id(id) {}

void Room::initialize_maps() {
    // 맵 생성
    auto mapA = std::make_shared<Map>("A", 300, 300);
    mapA->start_point = {1, 1};

    auto mapB = std::make_shared<Map>("B", 300, 300);
    mapB->start_point = {1, 1};

    auto mapC = std::make_shared<Map>("C", 300, 300);
    mapC->start_point = {1, 1};
    mapC->end_point = {299, 299};

    maps = {mapA, mapB, mapC};

    // 포탈 생성 및 연결
    portal_links[mapA->generate_random_portal(mapB->name)] = mapB;
    portal_links[mapB->generate_random_portal(mapC->name)] = mapC;
}

bool Room::add_player(std::shared_ptr<Player> player) {
    std::lock_guard<std::mutex> lock(mutex_); // 뮤텍스 잠금금
    if (is_full()) {
        return false;
    }
    players.push_back(player);
    player->room = shared_from_this();
    player->current_map = maps[0]; // 첫 번째 맵으로 설정정
    player->position = maps[0]->start_point;
    return true;
}

void Room::remove_player(std::shared_ptr<Player> player) {
    std::lock_guard<std::mutex> lock(mutex_); // 뮤텍스 잠금
    players.erase(std::remove(players.begin(), players.end(), player), players.end());
}

bool Room::is_full() const {
    return players.size() >= 30;
}