#include "connection_manager.hpp"

void ConnectionManager::register_connection(std::shared_ptr<Player> player, std::shared_ptr<Connection> connection) {
    std::lock_guard<std::mutex> lock(mutex_);
    player_to_connection_[player] = connection;
}

void ConnectionManager::unregister_connection(std::shared_ptr<Player> player) {
    std::lock_guard<std::mutex> lock(mutex_);
    player_to_connection_.erase(player);
}

std::shared_ptr<Connection> ConnectionManager::get_connection_for_player(std::shared_ptr<Player> player) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = player_to_connection_.find(player);
    return it != player_to_connection_.end() ? it->second : nullptr;
}

std::shared_ptr<Player> ConnectionManager::get_player_for_connection(std::shared_ptr<Connection> connection) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& pair : player_to_connection_) {
        if (pair.second == connection) {
            return pair.first;
        }
    }
    return nullptr;
}