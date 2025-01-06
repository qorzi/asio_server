#include "player.hpp"
#include "game_manager.hpp"
#include "connection_manager.hpp"
#include <iostream>

Player::Player(const std::string& id, const std::string& name) : id_(id), name_(name) {}

void Player::update_position(const Point& new_position) {
    position_ = new_position;
}

void Player::send_message(const std::string& message) {
    auto connection = ConnectionManager::get_instance().get_connection_for_player(shared_from_this());
    if (connection) {
        // Connection 객체를 통해 클라이언트로 메시지 전송
        connection->async_write(message);
        std::cout << "Message sent to Player [" << id_ << "]: " << message << std::endl;
    } else {
        std::cerr << "Failed to send message to Player [" << id_ << "]: Connection not found." << std::endl;
    }
}