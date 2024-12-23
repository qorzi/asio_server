#include "player.hpp"
#include <iostream>

Player::Player(const std::string& id) : id(id) {}

void Player::update_position(const Point& new_position) {
    position = new_position;
}
