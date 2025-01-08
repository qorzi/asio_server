#ifndef EVENT_HPP
#define EVENT_HPP

#include <boost/asio.hpp>
#include <vector>
#include <memory>
#include <optional>
#include <string>

// 전방 선언
class Connection;

enum class MainEventType : uint16_t {
    NETWORK = 1,
    GAME    = 2,
    ERROR   = 3,
    // ... etc
};

enum class NetworkSubType : uint16_t {
    JOIN  = 101,
    LEFT  = 102,
    CLOSE = 103,
    // ... etc
};

enum class GameSubType : uint16_t {
    ROOM_CREATE          = 201, // 방 생성
    GAME_COUNTDOWN       = 202, // 대기화면 후, 카운트다운
    GAME_START           = 203, // 카운트다운=0 → 게임 시작
    PLAYER_MOVED         = 204, // 플레이어가 이동
    PLAYER_COME_IN_MAP   = 205, // 플레이어가 맵에 입장
    PLAYER_COME_OUT_MAP  = 206, // 플레이어가 맵에서 나감
    PLAYER_FINISHED      = 207, // 플레이어가 도착
    GAME_END             = 208, // 게임 종료
    // ... etc
};

enum class ErrorSubType : uint16_t {
    UNKNOWN = 301, // 알 수 없는 에러
    // ... etc
};

// “통합” 이벤트 구조
struct Event {
    MainEventType main_type;      // NETWORK or GAME
    uint16_t sub_type;            // NetworkSubType or GameSubType

    // 네트워크 이벤트라면 connection이 있을 수 있음
    std::optional<std::weak_ptr<Connection>> connection;

    // 추가 데이터: json/문자열/room_id 등
    std::vector<char> data; 
    int room_id = -1;        // 룸 식별자
    std::string player_id;   // 플레이어 식별자
};

#endif // EVENT_HPP
