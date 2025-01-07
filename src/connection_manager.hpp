#ifndef CONNECTION_MANAGER_HPP
#define CONNECTION_MANAGER_HPP

#include <unordered_map>
#include <memory>
#include <mutex>
#include "player.hpp"
#include "connection.hpp"

class ConnectionManager {
public:
    static ConnectionManager& get_instance() {
        static ConnectionManager instance;
        return instance;
    }

    // 복사 및 이동 생성자 삭제
    ConnectionManager(const ConnectionManager&) = delete;
    ConnectionManager& operator=(const ConnectionManager&) = delete;
    ConnectionManager(ConnectionManager&&) = delete;
    ConnectionManager& operator=(ConnectionManager&&) = delete;

    // 메서드 정의
    void register_connection(std::shared_ptr<Player> player, std::shared_ptr<Connection> connection);
    void unregister_connection(std::shared_ptr<Player> player);
    std::shared_ptr<Connection> get_connection_for_player(std::shared_ptr<Player> player);
    std::shared_ptr<Player> get_player_for_connection(std::shared_ptr<Connection> connection) const;

private:
    // private 생성자
    ConnectionManager() = default;
    ~ConnectionManager() = default;

    // 데이터 멤버
    std::unordered_map<std::shared_ptr<Player>, std::shared_ptr<Connection>> player_to_connection_;
    mutable std::mutex mutex_; // 동시 접근 제어
};

#endif // CONNECTION_MANAGER_HPP