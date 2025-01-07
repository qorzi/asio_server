#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "point.hpp"
#include <memory>
#include <string>

class Room;
class Map;

class Player : public std::enable_shared_from_this<Player> {
public:
    std::string id_;                   // 플레이어 ID
    std::string name_;                 // 플레이어 이름
    Point position_;                   // 현재 위치
    std::weak_ptr<Room> room_;         // 연결된 룸
    std::shared_ptr<Map> current_map_; // 현재 맵

    explicit Player(const std::string& id, const std::string& name);
    void update_position(const Point& new_position); // 위치 업데이트
    void send_message(const std::string& message);   // 메시지 전송
};

#endif // PLAYER_HPP