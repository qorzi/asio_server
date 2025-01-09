// tests/test_packet.cpp

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <cstring>    // for std::memcpy
#include <cstdint>
#include <iostream>
#include "../src/utils.hpp"        // Utils::create_response_string(...)
#include "../src/header.hpp"       // Header, MainEventType, etc.

// 아래는, 클라이언트쪽 parse_packet(...)과 동일/유사 로직을 인메모리로 테스트할 함수
namespace {

struct ParsedResult {
    MainEventType main_type;
    uint16_t      sub_type;
    std::string   json_body;
};

// 간단히 8바이트 헤더( MainEventType(2) + sub_type(2) + body_length(4) )를 파싱하고,
// body_length만큼 JSON string을 읽은 뒤, '\0' 패딩 제거 & return
ParsedResult parse_in_memory(const std::vector<char>& packetData)
{
    // packetData.size() >= 8 이라고 가정 (테스트 시, 반드시 8 이상)
    // 1) 헤더 추출
    if (packetData.size() < 8) {
        throw std::runtime_error("Invalid packet size < 8");
    }

    // Header: < MainEventType(2 bytes), sub_type(2 bytes), body_length(4 bytes) >
    // 리틀엔디안(unless server is using network order explicitly)
    // 여기서는 server 코드가 <HHI> 리틀엔디안 형식이라고 가정

    // headerData[0..1] => main_type
    // headerData[2..3] => sub_type
    // headerData[4..7] => body_len (uint32_t)
    uint16_t raw_main = 0;
    uint16_t raw_sub  = 0;
    uint32_t body_len = 0;

    // memcpy를 써서 언패킹하거나, 직접 비트 연산도 가능
    std::memcpy(&raw_main, &packetData[0], 2);
    std::memcpy(&raw_sub,  &packetData[2], 2);
    std::memcpy(&body_len, &packetData[4], 4);

    // enum cast
    MainEventType main_type = static_cast<MainEventType>(raw_main);

    // 2) body 부분 추출
    if (packetData.size() < 8 + body_len) {
        throw std::runtime_error("Invalid packet: not enough size for body");
    }
    std::vector<char> bodyData(packetData.begin()+8, packetData.begin()+8+body_len);

    // 패딩 제거 ('\0')
    while (!bodyData.empty() && bodyData.back() == '\0') {
        bodyData.pop_back();
    }

    // JSON 문자열
    std::string json_str(bodyData.begin(), bodyData.end());

    // 반환
    ParsedResult result;
    result.main_type = main_type;
    result.sub_type  = raw_sub;
    result.json_body = json_str;
    return result;
}

} // anonymous namespace


TEST(PacketSerializationTest, SimpleJoinPacket)
{
    // 1) 서버쪽에서 직렬화한다고 가정
    //    예: main_type = NETWORK(1), sub_type=JOIN(101)
    //    바디 = R"({"action":"join","result":true})"
    //    Utils::create_response_string(...) 이용
    std::string bodyJson = R"({"action":"join","result":true})";
    std::string packetStr = 
        Utils::create_response_string(MainEventType::NETWORK, /*sub_type=*/101, bodyJson);

    // packetStr는 헤더(8바이트) + 패딩된 JSON 본문이 합쳐진 바이너리
    // parse_in_memory로 분석
    std::vector<char> packetData(packetStr.begin(), packetStr.end());
    ParsedResult parsed = parse_in_memory(packetData);

    // 2) 검증
    EXPECT_EQ(parsed.main_type, MainEventType::NETWORK);
    EXPECT_EQ(parsed.sub_type,   101);

    // JSON이 원본과 동일한지 검사
    // (패딩을 제거했으므로, 원래 bodyJson과 같아야 함)
    EXPECT_EQ(parsed.json_body, bodyJson);
}


TEST(PacketSerializationTest, GameStartPacket)
{
    // 1) 서버가 game_start를 브로드캐스트한다고 가정
    //    main_type = GAME(2), sub_type = GAME_START(203)
    //    body = R"({"action":"game_start","result":true})"
    std::string body = R"({"action":"game_start","result":true})";
    std::string packetStr = 
        Utils::create_response_string(MainEventType::GAME, /*sub_type=*/203, body);

    // 2) parse
    std::vector<char> packetData(packetStr.begin(), packetStr.end());
    ParsedResult parsed = parse_in_memory(packetData);

    EXPECT_EQ(parsed.main_type, MainEventType::GAME);
    EXPECT_EQ(parsed.sub_type,   203);
    EXPECT_EQ(parsed.json_body,  body);
}


TEST(PacketSerializationTest, BodyWithPadding)
{
    // JSON 바디 길이가 1~7 byte 등으로, 8의 배수가 아닐 때 패딩이 생긴다는 걸 확인
    // 예: body = "ABC" (3 bytes)
    // main_type=GAME=2, sub_type=204
    std::string body = "ABC"; // 3 bytes
    std::string packetStr =
        Utils::create_response_string(MainEventType::GAME, 204, body);

    std::vector<char> packetData(packetStr.begin(), packetStr.end());
    ParsedResult parsed = parse_in_memory(packetData);

    EXPECT_EQ(parsed.main_type, MainEventType::GAME);
    EXPECT_EQ(parsed.sub_type,   204);
    // body가 "ABC"와 동일해야 함(패딩 제거 후)
    EXPECT_EQ(parsed.json_body,  "ABC");
}


TEST(PacketSerializationTest, LargeJsonPacket)
{
    // 큰 JSON 예시
    std::string bigJson = R"({
       "action": "room_create",
       "maps": [
         {"name":"A","width":300,"height":300,"start":{"x":1,"y":1},"end":{"x":0,"y":0},
          "portals":[{"x":123,"y":45,"name":"A-1","linked_map":"B"}]
         },
         {"name":"B","width":300,"height":300,"start":{"x":1,"y":1},"end":{"x":0,"y":0},
          "portals":[{"x":67,"y":89,"name":"B-1","linked_map":"C"}]
         }
       ],
       "result":true
    })";

    std::string packetStr = 
        Utils::create_response_string(MainEventType::GAME, 201, bigJson);

    std::vector<char> packetData(packetStr.begin(), packetStr.end());
    ParsedResult parsed = parse_in_memory(packetData);

    EXPECT_EQ(parsed.main_type, MainEventType::GAME);
    EXPECT_EQ(parsed.sub_type,   201);
    // JSON 문자열이 완전히 동일한지
    EXPECT_EQ(parsed.json_body,  bigJson);
}
