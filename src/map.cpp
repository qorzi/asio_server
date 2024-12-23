#include "map.hpp"

Map::Map(const std::string& name) : name(name) {}

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
