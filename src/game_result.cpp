#include "game_result.hpp"

GameResult::GameResult(int room_id) : room_id_(room_id) {}

void GameResult::set_game_start_time() {
    std::lock_guard<std::mutex> lock(result_mutex_);
    auto start_time = std::chrono::system_clock::now();
    game_start_time_ = start_time;
}

void GameResult::set_game_end_time() {
    std::lock_guard<std::mutex> lock(result_mutex_);
    auto end_time = std::chrono::system_clock::now();
    game_end_time_ = end_time;
    // 진행 시간(초)을 계산
    play_duration_seconds_ = static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(game_end_time_ - game_start_time_).count());
}

void GameResult::add_player_result(const std::shared_ptr<Player>& player) {
    std::lock_guard<std::mutex> lock(result_mutex_);
    results_.emplace_back(PlayerResult{
        current_rank_++,
        player->id_,
        player->name_,
        player->total_distance_
    });
}

nlohmann::json GameResult::to_json() const {
    std::lock_guard<std::mutex> lock(result_mutex_);
    nlohmann::json result_json = {
        {"room_id", room_id_},
        {"results", nlohmann::json::array()}
    };
    for (const auto& result : results_) {
        result_json["results"].push_back(result.to_json());
    }
    return result_json;
}
