#include "map.hpp"

Map::Map(const std::string& name, int width, int height)
    : name(name), max_width(width), max_height(height) {}

std::string Map::generate_random_portal() {
    Point portal;
    do {
        portal = {rand() % max_width + 1, rand() % max_height + 1};
    } while ((portal == start_point) || (portal == end_point) ||
        std::find(portals.begin(), portals.end(), portal) != portals.end());

    portals.push_back(portal);
    return name + "-" + std::to_string(portals.size());
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