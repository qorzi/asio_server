#include "game_result.hpp"

GameResult::GameResult(int room_id) : room_id_(room_id) {}

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
