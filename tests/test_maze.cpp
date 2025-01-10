#include <gtest/gtest.h>
#include "map.hpp"
#include <queue>

/**
 * BFS(너비 우선 탐색)를 사용하여 시작점에서 목표 지점까지의 경로가 존재하는지 확인하는 헬퍼 함수.
 */
bool is_path_available(const Map& map, const Point& start, const Point& target) {
    const std::vector<Point> directions = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
    std::queue<Point> to_visit;
    std::set<Point> visited;

    to_visit.push(start);
    visited.insert(start);

    while (!to_visit.empty()) {
        Point current = to_visit.front();
        to_visit.pop();

        if (current == target) {
            return true; // 목표 지점에 도달 가능한 경로를 찾음
        }

        for (const auto& dir : directions) {
            Point neighbor = {current.x + dir.x, current.y + dir.y};

            if (map.is_valid_position(neighbor) && visited.find(neighbor) == visited.end()) {
                to_visit.push(neighbor);
                visited.insert(neighbor);
            }
        }
    }

    return false; // 목표 지점에 도달 가능한 경로가 없음
}

/**
 * Map 클래스와 generate_random_obstacles 함수에 대한 테스트.
 */
TEST(MapTest, GenerateRandomObstacles_PathValidation) {
    for (int i = 0; i < 10; ++i) {
        // 랜덤한 맵 크기 생성
        int width = rand() % 50 + 10; // 너비: 10~60
        int height = rand() % 50 + 10; // 높이: 10~60

        // 맵 생성
        Map map("TestMap", width, height);
        map.start_point = {1, 1};

        // 마지막 맵인지 랜덤 결정
        bool is_end = (i % 2 == 0);

        if (is_end) {
            map.end_point = {width, height};
        } else {
            // 포탈 생성
            map.portals_.push_back({{width / 2, height / 2}, "Portal-1", "LinkedMap"});
        }

        // 장애물 생성 및 경로 검증
        map.generate_random_obstacles(is_end);

        if (is_end) {
            // 종료 지점으로의 경로가 존재하는지 확인
            ASSERT_TRUE(is_path_available(map, map.start_point, map.end_point))
                << "맵 크기 " << width << "x" << height << "에서 end_point로의 경로가 존재하지 않습니다.";
        } else {
            // 포탈로의 경로가 존재하는지 확인
            ASSERT_TRUE(is_path_available(map, map.start_point, map.portals_.front().position))
                << "맵 크기 " << width << "x" << height << "에서 portal로의 경로가 존재하지 않습니다.";
        }
    }
}