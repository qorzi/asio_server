#include "game_manager.hpp"
#include <algorithm>
#include <iostream>

GameManager::GameManager() {}
GameManager::~GameManager() {}

// 대기열
void GameManager::add_waiting_player(std::shared_ptr<Player> p)
{
    std::lock_guard<std::mutex> lock(waiting_mtx_);
    waiting_players_.push_back(p);
}

bool GameManager::remove_waiting_player(std::shared_ptr<Player> p)
{
    std::lock_guard<std::mutex> lock(waiting_mtx_);
    auto it = std::find(waiting_players_.begin(), waiting_players_.end(), p);
    if (it != waiting_players_.end()) {
        waiting_players_.erase(it);
        return true;
    }
    return false;
}

std::vector<std::shared_ptr<Player>> GameManager::pop_waiting_players(std::size_t count)
{
    std::lock_guard<std::mutex> lock(waiting_mtx_);
    if (count > waiting_players_.size()) count = waiting_players_.size();
    std::vector<std::shared_ptr<Player>> to_move(
        waiting_players_.begin(),
        waiting_players_.begin() + count
    );
    waiting_players_.erase(waiting_players_.begin(), waiting_players_.begin() + count);
    return to_move;
}

size_t GameManager::waiting_count() const
{
    std::lock_guard<std::mutex> lock(waiting_mtx_);
    return waiting_players_.size();
}

// rooms
std::shared_ptr<Room> GameManager::create_room(int room_id)
{
    auto r = std::make_shared<Room>(room_id);
    std::lock_guard<std::mutex> lock(rooms_mtx_);
    rooms_.push_back(r);
    return r;
}

std::shared_ptr<Room> GameManager::find_room(int room_id)
{
    std::lock_guard<std::mutex> lock(rooms_mtx_);
    for (auto& r : rooms_) {
        if (r->id_ == room_id) return r;
    }
    return nullptr;
}

void GameManager::remove_room(int room_id)
{
    std::lock_guard<std::mutex> lock(rooms_mtx_);
    auto it = std::remove_if(rooms_.begin(), rooms_.end(), [room_id](auto& r){
        return (r->id_ == room_id);
    });
    if (it != rooms_.end()) {
        rooms_.erase(it, rooms_.end());
    }
}

std::vector<std::shared_ptr<Room>> GameManager::get_all_rooms() const
{
    std::lock_guard<std::mutex> lock(rooms_mtx_);
    return rooms_; // copy
}
