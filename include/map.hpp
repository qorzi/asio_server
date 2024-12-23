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
    std::string name;                     // 맵 이름 (예: A, B, C)
    Point start_point;                    // 시작 포인트
    Point end_point;                      // 엔드 포인트 (마지막 맵만 해당)
    std::vector<Point> portals;           // 포탈 위치 (다른 맵으로 이동)
    std::unordered_map<std::string, std::shared_ptr<Map>> connected_maps; // 각 포탈에 연결된 맵
    int max_width;                        // 맵 최대 가로 크기
    int max_height;                       // 맵 최대 세로 크기

    Map(const std::string& name, int width, int height);
    void generate_random_portals(int num_portals);         // 포탈 랜덤 생성
    bool is_portal(const Point& position) const;           // 현재 위치가 포탈인지 확인
    bool is_valid_position(const Point& position) const;   // 유요한 위치인지 확인
};

#endif // MAP_HPP