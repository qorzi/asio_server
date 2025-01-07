#ifndef GAME_MANAGER_HPP
#define GAME_MANAGER_HPP

#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include "room.hpp"
#include "player.hpp"

/**
 * GameManager
 *  - “상태 저장소” 역할
 *  - 대기열(waiting_players_)과 rooms_ 목록
 *  - "로직"은 하지 않고, 단순 get/set + thread-safe
 */
class GameManager {
public:
    GameManager();
    ~GameManager();

    // 대기열(waiting_players_)
    void add_waiting_player(std::shared_ptr<Player> p);
    bool remove_waiting_player(std::shared_ptr<Player> p);
    std::vector<std::shared_ptr<Player>> pop_waiting_players(std::size_t count);
    size_t waiting_count() const;

    // rooms
    std::shared_ptr<Room> create_room(int room_id);
    std::shared_ptr<Room> find_room(int room_id);
    void remove_room(int room_id);
    std::vector<std::shared_ptr<Room>> get_all_rooms() const;
    int current_room_id = 0;

private:
    mutable std::mutex waiting_mtx_;
    std::vector<std::shared_ptr<Player>> waiting_players_;

    mutable std::mutex rooms_mtx_;
    std::vector<std::shared_ptr<Room>> rooms_;
};

#endif // GAME_MANAGER_HPP
