#ifndef ROOM_HPP
#define ROOM_HPP

#include "map.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <condition_variable>

class Player;

class Room : public std::enable_shared_from_this<Room> {
public:
    const int id;                                                            // 룸 ID

    explicit Room(int id);

    // 플레이어 관련 함수
    bool add_player(std::shared_ptr<Player> player);                         // 플레이어 추가
    bool remove_player(std::shared_ptr<Player> player);                      // 플레이어 제거
    const std::vector<std::shared_ptr<Player>>& get_players() const;         // 플레이어 정보 반환

    // 타이머 관련 함수
    void start_timer(std::function<void(int, bool)> on_timer_expired,        // 타이머 실행
                    int wait_time_ms = 30000,  // 30초 (기본값)
                    int check_interval_ms = 1000); // 1초 (기본값)
    void stop_timer();                                                       // 타이머 종료
    const bool is_timer_active() const;                                      // 타이머 활성화 상태

    // 메시지 관련 함수
    void broadcast_message(const std::string& message);                      // 메시지 브로드캐스트 함수(전체에게 정보 전달달)

private:
    std::vector<std::shared_ptr<Map>> maps;                                  // 전체 맵 정보
    std::unordered_map<std::string, std::shared_ptr<Map>> portal_links;      // 포탈 연결 정보
    std::vector<std::shared_ptr<Player>> players;                            // 룸에 연결된 플레이어
    std::mutex mutex_;                                                       // 데이터 경합 방지를 위한 뮤텍스
    std::condition_variable cv_;

    std::atomic<bool> timer_active;                                          // 타이머 활성성 상태
    bool should_stop_ = false;                                               // 타이머 종료(중지) 플래그

    bool is_full() const;                                                    // 룸이 꽉 찼는지 확인
    void initialize_maps();                                                  // 맵과 포탈 초기화
    void start_game();
    void update_game_state();
};

#endif // ROOM_HPP