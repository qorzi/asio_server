#include "room.hpp"
#include <iostream>
#include <algorithm>

Room::Room(int id)
    : id_(id)
{
    std::cout << "[DEBUG][Room:" << id_ << "] Room constructor called.\n";
}

void Room::initialize_maps() {
    // 맵 생성
    auto mapA = std::make_shared<Map>("A", 15, 15);
    mapA->start_point = {1, 1};

    auto mapB = std::make_shared<Map>("B", 15, 15);
    mapB->start_point = {1, 1};

    auto mapC = std::make_shared<Map>("C", 15, 15);
    mapC->start_point = {1, 1};
    mapC->end_point   = {14, 14};

    // 포탈 생성
    mapA->generate_random_portal("B");
    mapB->generate_random_portal("C");
    
    // 장애물 생성
    mapA->generate_random_obstacles(false);
    mapB->generate_random_obstacles(false);
    mapC->generate_random_obstacles(true);

    {
        std::lock_guard<std::mutex> lock(room_mutex_);
        maps_.push_back(mapA);
        maps_.push_back(mapB);
        maps_.push_back(mapC);
    }
}

/**
 * 방에 참가시키면 "시작 맵" (maps_[0])에 배정
 * (핸들러 로직에서 호출)
 */
bool Room::join_player(std::shared_ptr<Player> player)
{
    std::lock_guard<std::mutex> lock(room_mutex_);
    if (maps_.empty()) {
        std::cerr << "[Room:" << id_ << "] no maps, cannot join.\n";
        return false;
    }
    // 시작 맵(예: 첫 맵)
    auto start_map = maps_[0];
    bool ok = start_map->add_player(player);
    if (ok) {
        player->current_map_ = start_map; // 플레이어 현재 맵
        player->position_ = start_map->start_point;
        player->room_id_ = id_;
        // 디버그 메시지
        std::cout << "[Room:" << id_ << "] Player " << player->id_ << " joined start_map=" 
                  << start_map->name << "\n";
    }
    return ok;
}

/**
 * 방 전체에서 특정 플레이어 찾기
 * (모든 맵을 뒤져서 검색)
 */
std::shared_ptr<Player> Room::find_player(const std::string& player_id)
{
    std::lock_guard<std::mutex> lock(room_mutex_);
    for (auto& m : maps_) {
        auto p = m->find_player(player_id);
        if (p) return p;
    }
    return nullptr;
}

/**
 * 플레이어가 게임에서 이탈(LEFT / CLOSE) 시
 * 방 안 모든 맵에서 제거
 */
bool Room::remove_player(std::shared_ptr<Player> player)
{
    std::lock_guard<std::mutex> lock(room_mutex_);
    bool removed = false;
    for (auto& m : maps_) {
        bool r = m->remove_player(player);
        if(r) {
            std::cout << "[Room:" << id_ << "] Removed player " 
                      << player->id_ << " from map " << m->name << "\n";
            removed = true;
        }
    }
    return removed;
}

/**
 * 방 전체 브로드캐스트:
 * 모든 맵에 있는 플레이어에게 메시지 전송
 */
void Room::broadcast_message(const std::string& message)
{
    std::lock_guard<std::mutex> lock(room_mutex_);
    for (auto& m : maps_) {
        m->broadcast_in_map(message);
    }
}

/**
 * 특정 맵 이름으로 검색
 */
std::shared_ptr<Map> Room::get_map_by_name(const std::string& name)
{
    std::lock_guard<std::mutex> lock(room_mutex_);
    auto it = std::find_if(maps_.begin(), maps_.end(), [&](auto& mm){
        return (mm->name == name);
    });
    if(it != maps_.end()) return *it;
    return nullptr;
}


/**
 * 룸의 모든 맵 정보를 JSON으로 구성
 * {
 *   "room_id": 2,
 *   "maps": [
 *     { ...mapA info... },
 *     { ...mapB info... },
 *     { ...mapC info... }
 *   ]
 * }
 */
nlohmann::json Room::extracte_all_map_info() const
{
    nlohmann::json j;
    j["room_id"] = id_;

    nlohmann::json maps_array = nlohmann::json::array();
    {
        std::lock_guard<std::mutex> lock(room_mutex_);
        for(auto& m : maps_) {
            maps_array.push_back(m->extracte_map_info());
        }
    }
    j["maps"] = maps_array;
    return j;
}