#ifndef GAME_RESULT_HPP
#define GAME_RESULT_HPP

#include "game_result.hpp"
#include "player.hpp"
#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <nlohmann/json.hpp>

/**
 * GameResult
 *  - 게임 결과를 저장
 *  - 플레이어의 순위, ID, 이름, 총 거리 등을 포함
 *  - JSON 형식으로 변환 가능
 */
class GameResult {
public:
    struct PlayerResult {
        int rank;                 // 플레이어 순위
        std::string player_id;    // 플레이어 ID
        std::string player_name;  // 플레이어 이름
        int total_distance;       // 총 이동 거리

        // JSON 변환
        nlohmann::json to_json() const {
            return {
                {"rank", rank},
                {"player_id", player_id},
                {"player_name", player_name},
                {"total_distance", total_distance}
            };
        }
    };

    explicit GameResult(int room_id);

    // 게임 시작 시간을 기록
    void set_game_start_time();
    // 게임 종료 시간을 기록하고, 진행 시간(초 단위) 계산
    void set_game_end_time();

    // 플레이어 결과 추가 (순서대로 rank 부여)
    void add_player_result(const std::shared_ptr<Player>& player);

    // JSON으로 변환
    nlohmann::json to_json() const;

private:
    int room_id_;                         // 방 ID
    int current_rank_ = 1;                // 현재 순위
    std::vector<PlayerResult> results_;   // 플레이어 결과 목록
    mutable std::mutex result_mutex_;     // 동기화

    // 게임 관련 시간 정보 (타임스탬프)
    std::chrono::system_clock::time_point game_start_time_;
    std::chrono::system_clock::time_point game_end_time_;
    int play_duration_seconds_ = 0;       // 진행 시간(초)

};

#endif // GAME_RESULT_HPP
