#ifndef SERVER_HPP
#define SERVER_HPP

#include "room.hpp"
#include "player.hpp"
#include <vector>
#include <memory>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

class Server {
public:
    static Server& getInstance();                           // 싱글톤 인스턴스 반환

    void initialize_game();                                 // 게임 초기화 및 첫 룸 생성
    void add_player_to_room(std::shared_ptr<Player> player);// 유저를 현재 룸에 추가
    void start_room_timer();                                // 룸 타이머 시작
    std::shared_ptr<Room> get_current_room() const;         // 현재 활성화 룸 정보 반환

    // 방과 관련된 브로드캐스팅 함수
    void broadcast_to_room(int room_id, const std::string& message);

private:
    Server();                                               // 생성자 private
    ~Server();                                              // 소멸자 private

    Server(const Server&) = delete;                         // 복사 생성자 삭제
    Server& operator=(const Server&) = delete;              // 복사 할당 연산자 삭제

    std::shared_ptr<Room> current_room;                     // 현재 활성화된 룸
    std::vector<std::shared_ptr<Room>> rooms;               // 생성된 모든 룸
    std::mutex room_mutex;                                  // 룸 생성 및 접근을 위한 뮤텍스
    std::condition_variable room_cv;                        // 타이머를 위한 조건 변수
    std::atomic<bool> room_timer_active;                    // 타이머 활성화 플래그
};

#endif // SERVER_HPP
