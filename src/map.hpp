#ifndef MAP_HPP
#define MAP_HPP

#include "point.hpp"
#include "player.hpp"
#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include <nlohmann/json.hpp>

/**
 * Map
 *  - 맵 내 플레이어 목록
 *  - 포탈, 장애물, start/end 등 지형 정보
 *  - 맵의 경계는 '벽'으로 사용되지 않음(0, max_width, max_height)
 */
struct Portal {
    Point position;
    std::string name;
    std::string linked_map_name;
};

struct Obstacle {
    Point position;

    bool operator==(const Point& other) const {
        return position == other;
    }
};

class Map : public std::enable_shared_from_this<Map> {
public:
    std::string name;
    Point start_point = {-1, -1};
    Point end_point = {-1, -1};
    int max_width;
    int max_height;

    // 포탈/장애물 등
    std::vector<Portal> portals_;
    std::vector<Obstacle> obstacles_;

    // 생성자
    Map(const std::string& name, int width, int height);

    // 포탈
    std::string generate_random_portal(const std::string& linked_map_name);
    bool is_portal(const Point& pos) const;

    // 장애물
    void generate_random_obstacles(bool is_end);
    bool is_paths_connected(const Point& start, const Point& target);
    bool is_obstacle(const Point& pos) const;

    // 이동 가능 확인
    bool is_valid_position(const Point& pos) const;

    // 플레이어 관리
    bool add_player(std::shared_ptr<Player> p);
    bool remove_player(std::shared_ptr<Player> p);
    std::shared_ptr<Player> find_player(const std::string& player_id);

    // 맵 정보 추출 함수 (to json)
    nlohmann::json extracte_map_info() const;

    // 맵 내부 브로드캐스트
    void broadcast_in_map(const std::string& msg);

private:
    std::mutex map_mutex_;
    std::vector<std::shared_ptr<Player>> map_players_;
    // TODO: 벽(Obstacle) 목록, ex: std::vector<Rect> obstacles_;
    //       bool is_blocked(Point pos) etc.
};

#endif
