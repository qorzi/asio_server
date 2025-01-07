#ifndef HEADER_HPP
#define HEADER_HPP

#include <vector>
#include <string>
#include <cstdint>
#include <cstring>

// 헤더 구조체 정의 (8 byte)
#pragma pack(push, 1)
struct Header {
    MainEventType main_type;    // 2 byte
    uint16_t sub_type;          // 2 byte
    uint32_t body_length;       // 4 bytes
};
#pragma pack(pop)

#endif // HEADER_HPP