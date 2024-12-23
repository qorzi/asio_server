#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "point.hpp"
#include "room.hpp"
#include <memory>
#include <string>

class Player {
public:
    std::string id;                   // 플레이어 ID
    Point position;                   // 현재 위치
    std::shared_ptr<Room> room;       // 연결된 룸
    std::shared_ptr<Map> current_map; // 현재 맵

    explicit Player(const std::string& id);
    void update_position(const Point& new_position); // 위치 업데이트
};

#endif // PLAYER_HPP