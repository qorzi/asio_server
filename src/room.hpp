#ifndef ROOM_HPP
#define ROOM_HPP

#include "map.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>

class Player;

class Room : public std::enable_shared_from_this<Room> {
public:
    int id;                                                                  // 룸 ID

    explicit Room(int id);
    void initialize_maps();                                                  // 맵과 포탈 초기화
    bool add_player(std::shared_ptr<Player> player);                         // 클라이언트 추가
    void remove_player(std::shared_ptr<Player> player);                      // 클라이언트 제거
    const std::vector<std::shared_ptr<Player>>& get_players() const;         // 플레이어 정보 반환환

private:
    std::vector<std::shared_ptr<Map>> maps;                                  // 전체 맵 정보
    std::unordered_map<std::string, std::shared_ptr<Map>> portal_links;      // 포탈 연결 정보
    std::vector<std::shared_ptr<Player>> players;                            // 룸에 연결된 클라이언트
    std::mutex mutex_;                                                       // 데이터 경합 방지를 위한 뮤텍스

    bool is_full() const;                                                    // 룸이 꽉 찼는지 확인
};

#endif // ROOM_HPP