#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "point.hpp"
#include <memory>
#include <string>
#include <iostream>
#include <atomic>
#include <sstream>
#include <iomanip>

class Map;

class Player : public std::enable_shared_from_this<Player> {
public:
    std::string id_;
    std::string name_;
    int room_id_;
    Point position_;
    int total_distance_;
    bool is_finished_ = false;
    std::weak_ptr<Map> current_map_; // 현재 맵(약한 참조)

    Player(const std::string& name);

    void update_position(const Point& new_position);
    bool is_valid_position(const Point& pos) const;
    void send_message(const std::string& message);
    
private:
    // 고유 id 생성을 위한 정적 카운터
    static std::atomic<uint64_t> id_counter_;
};

#endif
