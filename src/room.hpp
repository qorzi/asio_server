#ifndef ROOM_HPP
#define ROOM_HPP

#include <boost/asio/steady_timer.hpp>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <functional>

#include "map.hpp"
#include "player.hpp"

// 전방 선언
class Room : public std::enable_shared_from_this<Room> {
public:
    const int id_;

    Room(int id);

    // Map 초기화 함수
    void initialize_maps();

    // 플레이어 관련 함수
    bool add_player(std::shared_ptr<Player> player);
    bool remove_player(std::shared_ptr<Player> player);
    std::shared_ptr<Player> find_player(const std::string& player_id);
    const std::vector<std::shared_ptr<Player>>& get_players() const;

    // 메시지 브로드캐스트
    void broadcast_message(const std::string& message);

private:
    std::vector<std::shared_ptr<Map>> maps_;
    std::unordered_map<std::string, std::shared_ptr<Map>> portal_links_;
    std::vector<std::shared_ptr<Player>> players_;
    std::mutex mutex_;
};

#endif // ROOM_HPP