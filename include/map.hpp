#ifndef MAP_HPP
#define MAP_HPP

#include "point.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdlib>

class Map {
public:
    std::string name;                     // 맵 이름 (A, B, C)
    Point start_point;                    // 시작 포인트
    Point end_point;                      // 엔드 포인트 (C맵만 해당)
    std::vector<Point> portals;           // 포탈 위치 (다른 맵으로 이동)
    std::unordered_map<std::string, std::shared_ptr<Map>> connected_maps; // 연결된 맵

    Map(const std::string& name);
    void generate_random_portals(int num_portals); // 포탈 랜덤 생성
    bool is_portal(const Point& position) const;   // 현재 위치가 포탈인지 확인
};

#endif // MAP_HPP