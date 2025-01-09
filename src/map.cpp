#include "map.hpp"
#include <algorithm>
#include <iostream>
#include <cstdlib> // rand

Map::Map(const std::string& name, int width, int height)
    : name(name)
    , max_width(width)
    , max_height(height)
{
}

/**
 * 맵에 랜덤 위치의 포탈 생성하기
 * 시작과 종료 위치를 제외한 위치에 생성
 */
std::string Map::generate_random_portal(const std::string& linked_map_name)
{
    Portal portal;
    do {
        portal.position = { rand()%max_width+1, rand()%max_height+1 };
    } while (portal.position == start_point 
          || portal.position == end_point 
          || std::any_of(portals.begin(), portals.end(), [&](const Portal& pp){
               return pp.position == portal.position; 
          }));

    portal.name = name + "-" + std::to_string(portals.size()+1);
    portal.linked_map_name = linked_map_name;
    portals.push_back(portal);
    return portal.name;
}

/**
 * 해당 포지션이 포탈 위치인지 확인
 * (플레이어 이동 시, 포탈인지 확인용)
 */
bool Map::is_portal(const Point& pos) const {
    for(auto& pt: portals){
        if(pt.position == pos) return true;
    }
    return false;
}

/**
 * 이동할 수 있는 위치인지 확인
 * 1) 도달할 수 있는 범위 인지 확인
 * 2) 이동 범위가 1인지 확인
 * TODO: 장애물이 있는지 확인하는 로직 추가
 */
bool Map::is_valid_position(const Point& pos) const {
    return (pos.x>0 && pos.y>0 && pos.x<=max_width && pos.y<=max_height);
}

/**
 * 맵에 특정 플레이어 추가
 * 
 */
bool Map::add_player(std::shared_ptr<Player> p)
{
    std::lock_guard<std::mutex> lock(map_mutex_);
    auto it = std::find(map_players_.begin(), map_players_.end(), p);
    if(it != map_players_.end()){
        return false; // already in this map
    }
    map_players_.push_back(p);
    // debug
    std::cout << "[Map:" << name << "] add_player " << p->id_ << ", total=" << map_players_.size() << "\n";
    return true;
}

/**
 * 맵에서 특정 플레이어 제거
 * (맵 이탈 시 사용)
 */
bool Map::remove_player(std::shared_ptr<Player> p)
{
    std::lock_guard<std::mutex> lock(map_mutex_);
    auto it = std::find(map_players_.begin(), map_players_.end(), p);
    if(it != map_players_.end()){
        map_players_.erase(it);
        std::cout << "[Map:" << name << "] remove_player " << p->id_ 
                  << ", total=" << map_players_.size() << "\n";
        return true;
    }
    return false;
}

/**
 * 맵에서 특정 플레이어 찾기
 * 
 */
std::shared_ptr<Player> Map::find_player(const std::string& player_id)
{
    std::lock_guard<std::mutex> lock(map_mutex_);
    for(auto& pl : map_players_){
        if(pl->id_ == player_id) return pl;
    }
    return nullptr;
}

/** 
 * 맵 정보를 JSON으로 구성
 * {
 *   "name": "A",
 *   "width": 300,
 *   "height":300,
 *   "start": {"x":1,"y":1},
 *   "end":   {"x":299,"y":299}, 
 *   "portals": [
 *       {"x":..., "y":..., "name":"A-1", "linked_map":"B"},
 *       ...
 *   ]
 * }
 */
nlohmann::json Map::extracte_map_info() const
{
    nlohmann::json map_info;
    map_info["name"]   = name;
    map_info["width"]  = max_width;
    map_info["height"] = max_height;

    // start
    {
        nlohmann::json st;
        st["x"] = start_point.x;
        st["y"] = start_point.y;
        map_info["start"] = st;
    }
    // end
    {
        nlohmann::json ed;
        ed["x"] = end_point.x;
        ed["y"] = end_point.y;
        map_info["end"] = ed;
    }

    // portals
    nlohmann::json portal_array = nlohmann::json::array();
    for (auto& pt : portals) {
        nlohmann::json pjson;
        pjson["x"] = pt.position.x;
        pjson["y"] = pt.position.y;
        pjson["name"] = pt.name;
        pjson["linked_map"] = pt.linked_map_name;
        portal_array.push_back(pjson);
    }
    map_info["portals"] = portal_array;

    // TODO: obstacles if any
    // map_info["obstacles"] = ...

    return map_info;
}

/**
 * 맵 전용 브로드캐스트
 * 해당 맵에 있는 플레이어에게 메시지 전송
 */
void Map::broadcast_in_map(const std::string& msg)
{
    std::lock_guard<std::mutex> lock(map_mutex_);
    for(auto& p : map_players_) {
        p->send_message(msg);
    }
}
