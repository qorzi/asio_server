#include "utils.hpp"
#include "header.hpp"

// 직렬화 헬퍼 함수
std::string Utils::create_response_string(MainEventType main_type, uint16_t sub_type, const std::string& body) {
    // 헤더 생성
    Header header{main_type, sub_type, static_cast<uint32_t>(body.size())};

    // 헤더를 바이너리 데이터로 변환(직렬화)
    std::vector<char> header_buffer(sizeof(Header));
    std::memcpy(header_buffer.data(), &header, sizeof(Header));

    // 본문 데이터 패딩 처리 (8바이트 배수로 맞춤)
    size_t padded_length = ((body.size() + 7) / 8) * 8; // 8의 배수로 맞춤
    std::string padded_body = body;
    padded_body.resize(padded_length, '\0'); // '\0'으로 패딩 추가

    // 헤더와 패딩된 본문 결합
    std::string response(header_buffer.begin(), header_buffer.end());
    response += padded_body;

    return response; // 결합된 헤더 + 패딩된 본문 스트링 반환
}