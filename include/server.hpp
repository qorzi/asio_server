#ifndef SERVER_HPP
#define SERVER_HPP

#include "room.hpp"
#include "client.hpp"
#include <vector>
#include <memory>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>

class Server {
public:
    std::shared_ptr<Room> current_room;                  // 현재 활성화된 룸
    std::vector<std::shared_ptr<Room>> rooms;            // 생성된 모든 룸
    std::mutex room_mutex;                               // 룸 생성 및 접근을 위한 뮤텍스
    std::condition_variable room_cv;                     // 타이머를 위한 조건 변수

    Server();
    void initialize_game();                              // 게임 초기화 및 첫 룸 생성
    void add_player_to_room(std::shared_ptr<Player> player); // 유저를 현재 룸에 추가
    void start_room_timer();                             // 룸 타이머 시작
};

#endif // SERVER_HPP