#include "map.hpp"

Map::Map(const std::string& name, int width, int height)
    : name(name), max_width(width), max_height(height) {}

std::string Map::generate_random_portal(const std::string& linked_map_name) {
    Portal portal;
    do {
        portal.position = {rand() % max_width + 1, rand() % max_height + 1};
    } while ((portal.position == start_point) || (portal.position == end_point) || 
             std::any_of(portals.begin(), portals.end(), [&](const Portal& p) { return p.position == portal.position; }));

    portal.name = name + "-" + std::to_string(portals.size() + 1);
    portal.linked_map_name = linked_map_name; // 연결된 맵 이름 저장
    portals.push_back(portal);
    return portal.name;
}

bool Map::is_portal(const Point& position) const {
    for (const auto& portal : portals) {
        if (portal.position == position) {
            return true;
        }
    }
    return false;
}

bool Map::is_valid_position(const Point& position) const {
    return position.x > 0 && position.y > 0 && position.x <= max_width && position.y <= max_height;
}