#ifndef SERVER_HPP
#define SERVER_HPP

#include "room.hpp"
#include "player.hpp"
#include "connection_manager.hpp"
#include <vector>
#include <memory>
#include <mutex>

class Server {
public:
    static Server& getInstance();                               // 싱글톤 인스턴스 반환

    void initialize_server();                                   // 게임 초기화 및 첫 룸 생성
    void add_player_to_room(std::shared_ptr<Player> player);    // 유저를 현재 룸에 추가
    void remove_player_to_room(std::shared_ptr<Player> player);
    std::shared_ptr<Room> get_current_room() const;             // 현재 활성화 룸 정보 반환

    ConnectionManager& get_connection_manager();                // ConnectionManager 반환
private:
    Server();                                                   // 생성자 private
    ~Server();                                                  // 소멸자 private

    Server(const Server&) = delete;                             // 복사 생성자 삭제
    Server& operator=(const Server&) = delete;                  // 복사 할당 연산자 삭제

    std::shared_ptr<Room> current_room;                         // 현재 활성화된 룸
    std::vector<std::shared_ptr<Room>> rooms;                   // 생성된 모든 룸
    std::mutex room_mutex;                                      // 룸 생성 및 접근을 위한 뮤텍스

    ConnectionManager connection_manager_;                      // ConnectionManager 멤버 추가

    void create_room();                                         // 룸 생성 함수
    void delete_room(int room_id);                              // 룸 제거 함수
    void on_room_timer_expired(int room_id, bool is_start);     // 타미어 만료 콜백
    void shutdown();                                            // 모든 룸 타이머 종료
};

#endif // SERVER_HPP
