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

    // Room은 반드시 io_context를 받아야 함
    Room(int id, boost::asio::io_context& ioc);

    // 플레이어 관련 함수
    bool add_player(std::shared_ptr<Player> player);
    bool remove_player(std::shared_ptr<Player> player);
    const std::vector<std::shared_ptr<Player>>& get_players() const;

    void start_game();

    // 메시지 브로드캐스트
    void broadcast_message(const std::string& message);

private:
    boost::asio::io_context& ioc_;      // Reactor와 공유할 io_context

    std::vector<std::shared_ptr<Map>> maps_;
    std::unordered_map<std::string, std::shared_ptr<Map>> portal_links_;
    std::vector<std::shared_ptr<Player>> players_;
    std::mutex mutex_;

    void initialize_maps();

    void update_game_state();  // 미구현
};

#endif // ROOM_HPP