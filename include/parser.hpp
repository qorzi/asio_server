#ifndef PARSER_HPP
#define PARSER_HPP

#include <vector>
#include <string>
#include <cstdint>

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

class Parser {
public:
    static Header parse_header(const std::vector<char>& header);
    static std::string process_body(const std::string& body);
};

#endif // PARSER_HPP