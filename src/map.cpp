#include "map.hpp"

Map::Map(const std::string& name, int width, int height)
    : name(name), max_width(width), max_height(height) {}

void Map::generate_random_portals(int num_portals) {
    portals.clear();
    for (int i = 0; i < num_portals; ++i) {
        portals.push_back({rand() % 300, rand() % 300});
    }
}

bool Map::is_portal(const Point& position) const {
    for (const auto& portal : portals) {
        if (portal == position) {
            return true;
        }
    }
    return false;
}

bool Map::is_valid_position(const Point& position) const {
    return position.x > 0 && position.y > 0 && position.x <= max_width && position.y <= max_height;
}