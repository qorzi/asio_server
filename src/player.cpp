#include "player.hpp"
#include "server.hpp"
#include <iostream>

Player::Player(const std::string& id, const std::string& name) : id(id), name(name) {}

void Player::update_position(const Point& new_position) {
    position = new_position;
}

void Player::send_message(const std::string& message) {
    // ConnectionManager를 통해 현재 플레이어와 연결된 Connection 가져오기
    Server& server = Server::getInstance();
    ConnectionManager& connection_manager = server.get_connection_manager();

    auto connection = connection_manager.get_connection_for_player(shared_from_this());
    if (connection) {
        // Connection 객체를 통해 클라이언트로 메시지 전송
        connection->async_write(message);
        std::cout << "Message sent to Player [" << id << "]: " << message << std::endl;
    } else {
        std::cerr << "Failed to send message to Player [" << id << "]: Connection not found." << std::endl;
    }
}