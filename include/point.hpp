#ifndef POINT_HPP
#define POINT_HPP

struct Point {
    int x;
    int y;

    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};

#endif // POINT_HPP