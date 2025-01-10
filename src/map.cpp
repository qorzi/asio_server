#include "map.hpp"
#include <algorithm>
#include <iostream>
#include <cstdlib> // rand
#include <stack>
#include <queue>
#include <set>
#include <random>

Map::Map(const std::string& name, int width, int height)
    : name(name)
    , max_width(width)
    , max_height(height)
{
}

/**
 * 맵에 랜덤 위치의 포탈 생성하기
 * 시작과 종료 위치를 제외한 위치에 생성
 * 포탈이 맵의 경계(0 or Max)에 생성 되지 않도록 함
 */
std::string Map::generate_random_portal(const std::string& linked_map_name)
{
    int min_distance = (max_width + max_height) / 2; // 최소 거리
    Portal portal;
    do {
        portal.position = { rand() % (max_width - 2) + 1, rand() % (max_height - 2) + 1 };
    } while (portal.position == start_point 
          || portal.position == end_point 
          || manhattan_distance(portal.position, start_point) < min_distance
          || std::any_of(portals_.begin(), portals_.end(), [&](const Portal& pp){
               return pp.position == portal.position; 
          }));

    portal.name = name + "-" + std::to_string(portals_.size()+1);
    portal.linked_map_name = linked_map_name;
    portals_.push_back(portal);
    return portal.name;
}

// 맨해튼 거리 계산 헬퍼 함수
int Map::manhattan_distance(const Point& a, const Point& b) const {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

/**
 * 해당 포지션이 포탈 위치인지 확인
 * (플레이어 이동 시, 포탈인지 확인용)
 */
bool Map::is_portal(const Point& pos) const {
    for(auto& pt: portals_){
        if(pt.position == pos) return true;
    }
    return false;
}

/**
 * 맵에 랜덤 위치릐 장애물 생성하기
 * 1) 마지막 맵인지 아닌지를 요소로 가져옴
 * 2) 마지막 맵은 end_point로 가는 길을 만들고,
 * 3) 아닌 경우, 포탈로 가는 길을 생성함
 * 4) 미로 생성을 위한 더미 도착지 추가가
 * 5) 길찾기 로직으로 미로 생성성
 * 6) 생성 된 길 외에는 전부 장애물로 설정
 */
void Map::generate_random_obstacles(bool is_end)
{
    // 방향 벡터: 상, 하, 좌, 우
    const std::vector<Point> directions = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};

    // 최대 재시도 횟수 설정 (예: 10회)
    const int MAX_ATTEMPTS = 10;
    int attempt = 0;

    Point main_target;

    do {
        if (attempt >= MAX_ATTEMPTS) {
            throw std::runtime_error("최대 재시도 횟수를 초과하여 맵을 생성할 수 없습니다.");
        }

        // 미로 초기화: 모든 위치를 장애물로 설정
        obstacles_.clear();
        for (int x = 1; x < max_width - 1; ++x) {
            for (int y = 1; y < max_height - 1; ++y) {
                obstacles_.push_back({x, y});
            }
        }

        // 방문 처리 유틸리티
        auto remove_obstacle = [&](const Point& pos) {
            auto it = std::find(obstacles_.begin(), obstacles_.end(), pos);
            if (it != obstacles_.end()) {
                obstacles_.erase(it);
            }
        };

        std::random_device rd;
        std::mt19937 rng(rd());

        // 1. 더미 도착지점 생성
        std::vector<Point> targets;
        if (is_end) {
            main_target = end_point;
            targets.push_back(end_point); // 종료 지점 포함
        } else {
            main_target = portals_.front().position;
            targets.push_back(portals_.front().position); // 포탈 포함 (첫번째 포탈만 포함)
        }

        int additional_targets = std::max(1, (max_width * max_height) / 70); // 맵 크기에 따라 더미 지점 생성
        while (targets.size() < additional_targets + 1) {
            Point dummy_target = {rand() % (max_width - 2) + 1, rand() % (max_height - 2) + 1};
            if (dummy_target != start_point && 
                dummy_target != end_point && 
                !is_portal(dummy_target) && 
                std::find(targets.begin(), targets.end(), dummy_target) == targets.end()) {
                targets.push_back(dummy_target);
            }
        }

        // 2. 각 도착지점으로 경로 생성
        for (const auto& target : targets) {
            std::stack<Point> stack;
            stack.push(start_point);
            remove_obstacle(start_point);

            while (!stack.empty()) {
                Point current = stack.top();

                // 현재 위치에서 이동 가능한 랜덤한 방향 선택
                std::vector<Point> neighbors;
                for (const auto& dir : directions) {
                    Point next = {current.x + dir.x, current.y + dir.y};
                    if (next.x > 0 && next.y > 0 && next.x < max_width && next.y < max_height &&
                        std::find(obstacles_.begin(), obstacles_.end(), next) != obstacles_.end()) {
                        neighbors.push_back(next);
                    }
                }

                if (!neighbors.empty()) {
                    // 랜덤한 이웃 선택
                    std::shuffle(neighbors.begin(), neighbors.end(), rng);
                    Point next = neighbors.front();

                    // 현재 위치와 다음 위치 사이의 길 제거
                    remove_obstacle(next);
                    stack.push(next);

                    if (next == target) {
                        break; // 목표에 도달하면 경로 생성 종료
                    }
                } else {
                    stack.pop();
                }
            }
        }

        attempt++; // 재시도 횟수 증가

    } while (!is_paths_connected(start_point, main_target));

    std::cout << "[Map] 맵 생성 완료." << "\n";
}

// 경로 연결 여부 확인 함수
bool Map::is_paths_connected(const Point& start, const Point& target)
{
    const std::vector<Point> directions = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};

    // BFS를 활용하여 경로 연결 여부 확인
    std::queue<Point> queue;
    std::set<Point> visited;

    queue.push(start);
    visited.insert(start);

    while (!queue.empty()) {
        Point current = queue.front();
        queue.pop();

        if (current == target) {
            return true; // 목표 지점에 도달 가능한 경로를 찾음
        }

        for (const auto& dir : directions) {
            Point next = {current.x + dir.x, current.y + dir.y};

            if (is_valid_position(next) && visited.find(next) == visited.end()) {
                queue.push(next);
                visited.insert(next);
            }
        }
    }

    std::cout << "[Map] 포탈 또는 종료 지점에 도달할 수 없습니다. 재생성합니다." << "\n";
    return false;
}

/**
 * 해당 포지션이 장애물 위치인지 확인
 * (플레이어 이동 시, 장애물인지 확인용)
 */
bool Map::is_obstacle(const Point& pos) const {
    for(auto& obs: obstacles_){
        if(obs.position == pos) return true;
    }
    return false;
}

/**
 * 이동할 수 있는 위치인지 확인
 * 1) 도달할 수 있는 범위 인지 확인
 * 2) 장애물이 있는지 확인
 */
bool Map::is_valid_position(const Point& pos) const {
    return (pos.x > 0 && pos.y > 0 && pos.x < max_width - 1 && pos.y < max_height - 1) 
           && !is_obstacle(pos);
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
 *      {"x":..., "y":..., "name":"A-1", "linked_map":"B"},
 *      ...
 *   ]
 *   "obstacles": [
 *      {"x":..., "y":...},
 *      ...
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
    for (auto& pt : portals_) {
        nlohmann::json pjson;
        pjson["x"] = pt.position.x;
        pjson["y"] = pt.position.y;
        pjson["name"] = pt.name;
        pjson["linked_map"] = pt.linked_map_name;
        portal_array.push_back(pjson);
    }
    map_info["portals"] = portal_array;

    // obstacles
    nlohmann::json obstacle_array = nlohmann::json::array();
    for (auto& obs : obstacles_) {
        nlohmann::json ojson;
        ojson["x"] = obs.position.x;
        ojson["y"] = obs.position.y;
        obstacle_array.push_back(ojson);
    }
    map_info["obstacles"] = obstacle_array;

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
