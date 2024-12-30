#include "player.hpp"
#include <iostream>

Player::Player(const std::string& id, const std::string& name) : id(id), name(name) {}

void Player::update_position(const Point& new_position) {
    position = new_position;
}

void Player::send_message(const std::string& message) {
    // 예: 콘솔에 메시지를 출력
    std::cout << "Message to Player [" << id << "]: " << message << std::endl;

    // 실제 네트워크 전송 로직을 구현하려면 여기에 추가
    // 예:
    // network_socket.send(id, message);
}