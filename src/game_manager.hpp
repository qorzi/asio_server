#ifndef GAME_MANAGER_HPP
#define GAME_MANAGER_HPP

#include <boost/asio/io_context.hpp>
#include <vector>
#include <memory>
#include <mutex>
#include "room.hpp"

class GameManager {
public:
    explicit GameManager(boost::asio::io_context& ioc);
    ~GameManager();

    void initialize();
    void shutdown();

    // 플레이어가 게임에 참가 신청할 때
    void join_game(std::shared_ptr<Player> player);
    // 방에 들어간 플레이어를 빼는 로직
    void remove_player_from_room(std::shared_ptr<Player> player);

private:
    // 대기 로직 관련
    void start_waiting_timer();
    void stop_waiting_timer();
    void on_waiting_timer_expired(const boost::system::error_code& ec);
    void try_create_room_if_ready();

    // 실제 룸 생성/삭제
    void create_room(const std::vector<std::shared_ptr<Player>>& players_to_move);
    void delete_room(int room_id);

    boost::asio::io_context& ioc_;

    // "대기중"인 플레이어 목록
    std::vector<std::shared_ptr<Player>> waiting_players_;
    std::mutex waiting_mutex_;

    // 대기 타이머 (30초)
    std::unique_ptr<boost::asio::steady_timer> waiting_timer_;
    bool waiting_timer_active_ = false;

    // 5명 이상이면 방 생성
    static constexpr int WAITING_ROOM_THRESHOLD = 5;
    static constexpr int WAITING_ROOM_TIMEOUT_MS = 30000;  // 30초 대기 후 종료

    // 실제 생성된 룸 목록
    std::vector<std::shared_ptr<Room>> rooms_;
    std::mutex room_mutex_;
};

#endif // GAME_MANAGER_HPP
