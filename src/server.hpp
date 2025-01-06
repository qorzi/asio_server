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
    Server();
    ~Server();

    void initialize_server();
    void shutdown();

    void add_player_to_room(std::shared_ptr<Player> player);
    void remove_player_to_room(std::shared_ptr<Player> player);

    std::shared_ptr<Room> get_current_room() const;

    ConnectionManager& get_connection_manager();

private:
    void create_room();
    void delete_room(int room_id);
    void on_room_timer_expired(int room_id, bool is_start);

    std::shared_ptr<Room> current_room_;
    std::vector<std::shared_ptr<Room>> rooms_;
    std::mutex room_mutex_;

    ConnectionManager connection_manager_;
};

#endif // SERVER_HPP
