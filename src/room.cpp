#include "room.hpp"
#include "player.hpp"

Room::Room(int id, const std::vector<std::shared_ptr<Map>>& maps) 
    : id(id), maps(maps) {}

bool Room::add_player(std::shared_ptr<Player> player) {
    if (is_full()) {
        return false;
    }
    players.push_back(player);
    player->room = shared_from_this();
    player->current_map = maps[0];
    player->position = maps[0]->start_point;
    return true;
}

void Room::remove_player(std::shared_ptr<Player> player) {
    players.erase(std::remove(players.begin(), players.end(), player), players.end());
}

bool Room::is_full() const {
    return players.size() >= 30;
}