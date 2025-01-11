#ifndef ROOM_HPP
#define ROOM_HPP

#include "map.hpp"
#include "player.hpp"
#include "game_result.hpp"
#include <memory>
#include <vector>
#include <mutex>
#include <nlohmann/json.hpp>

/**
 * Room
 *  - 여러 Map(A/B/C 등)을 보유
 *  - 플레이어 목록은 각 Map이 관리
 *  - 방 전체에서 "플레이어 찾기 / 제거" 등의 함수 제공
 */
class Room : public std::enable_shared_from_this<Room> {
public:
    const int id_;
    GameResult gr_;

    explicit Room(int id);

    // 맵 초기화 (A/B/C 생성, etc.)
    void initialize_maps();

    // 플레이어 "방 입장": 시작 맵(예: maps_[0])에 배정
    // (핸들러에서 편하게 사용)
    bool join_player(std::shared_ptr<Player> player);

    // 방 전체에서 플레이어를 찾음(모든 맵 검색)
    std::shared_ptr<Player> find_player(const std::string& player_id);

    // 방 전체에서 플레이어 제거(게임에서 이탈) 
    //  -> 어떤 맵에 있는지 찾아 remove_player
    bool remove_player(std::shared_ptr<Player> player);

    // 방 전체에서 모든 플레이어를 찾아 반환
    std::vector<std::shared_ptr<Player>> get_all_players() const;

    // 룸의 모든 플레이어가 경기를 마무리 했는지 확인
    bool is_all_players_finished() const;

    // 편의 함수: 방 전체에 broadcast (각 맵의 플레이어 전체)
    void broadcast_message(const std::string& message);

    // 맵 접근
    std::shared_ptr<Map> get_map_by_name(const std::string& name);

    // 맵 전체 정보 추출
    nlohmann::json extract_all_map_info() const;

private:
    std::vector<std::shared_ptr<Map>> maps_;
    mutable std::mutex room_mutex_; // protect maps_ if needed
};

#endif // ROOM_HPP
