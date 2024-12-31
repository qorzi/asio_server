#include <unordered_map>
#include <memory>
#include <mutex>
#include "player.hpp"
#include "connection.hpp"

class ConnectionManager {
public:
    void register_connection(std::shared_ptr<Player> player, std::shared_ptr<Connection> connection);
    void unregister_connection(std::shared_ptr<Player> player);
    std::shared_ptr<Connection> get_connection_for_player(std::shared_ptr<Player> player);
    std::shared_ptr<Player> get_player_for_connection(std::shared_ptr<Connection> connection) const;

private:
    std::unordered_map<std::shared_ptr<Player>, std::shared_ptr<Connection>> player_to_connection_;
    mutable std::mutex mutex_; // 동시 접근 제어
};
