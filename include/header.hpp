#ifndef HEADER_HPP
#define HEADER_HPP

#include <vector>
#include <string>
#include <cstdint>
#include <cstring>

// 요청 타입을 정의하는 enum
enum class RequestType : uint8_t {
    UNKNOWN = 0,
    GET = 1,
    POST = 2,
    DELETE = 3
};

// 헤더 구조체 정의
struct Header {
    RequestType type;       // 요청 타입
    uint32_t body_length;   // 본문 길이
};

#endif // HEADER_HPP