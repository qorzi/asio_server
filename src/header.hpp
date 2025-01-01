#ifndef HEADER_HPP
#define HEADER_HPP

#include <vector>
#include <string>
#include <cstdint>
#include <cstring>

// 요청 타입을 정의하는 enum
enum class RequestType : uint8_t {
    UNKNOWN = 0,
    JOIN = 1,
    LEFT = 2,
    INPUT = 3
};

// 헤더 구조체 정의
#pragma pack(push, 1)
struct Header {
    RequestType type;       // 1 byte
    uint32_t body_length;   // 4 bytes
    char padding[3];        // 패딩 추가
};
#pragma pack(pop)

#endif // HEADER_HPP