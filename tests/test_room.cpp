#include <gtest/gtest.h>
#include "server.hpp"
#include "player.hpp"
#include <thread> // std::this_thread::sleep_for
#include <chrono> // std::chrono::seconds

// 서버에 플레이어를 추가하고, 현재 활성화된 룸에 플레이어가 정상적으로 추가되었는지 확인
TEST(RoomTest, AddPlayerToRoom) {
    Server& server = Server::getInstance();
    server.initialize_server();

    // 새로운 플레이어 생성
    auto player1 = std::make_shared<Player>("player1", "Alice");
    auto player2 = std::make_shared<Player>("player2", "Bob");

    // 플레이어 추가
    server.add_player_to_room(player1);
    server.add_player_to_room(player2);

    // 현재 활성화된 룸 확인
    auto current_room = server.getInstance().get_current_room();
    EXPECT_EQ(current_room->get_players().size(), 2);          // 두 명의 플레이어가 추가되었는지 확인
    EXPECT_EQ(current_room->get_players()[0]->id, "player1");  // 첫 번째 플레이어 확인
    EXPECT_EQ(current_room->get_players()[1]->id, "player2");  // 두 번째 플레이어 확인
}

// 서버가 룸의 최대 인원을 초과하면 새로운 룸을 생성하는지 확인
TEST(RoomTest, CreateNewRoomWhenFull) {
    Server& server = Server::getInstance();
    server.initialize_server();

    // 현재 룸에 최대 인원(30명) 추가
    for (int i = 0; i < 30; ++i) {
        server.add_player_to_room(std::make_shared<Player>("player" + std::to_string(i), "Player" + std::to_string(i)));
    }

    // 새로운 플레이어 추가 -> 새로운 룸 생성 확인
    auto new_player = std::make_shared<Player>("player31", "NewPlayer");
    server.add_player_to_room(new_player);

    auto current_room = server.getInstance().get_current_room();
    EXPECT_EQ(current_room->get_players().size(), 1);          // 새 룸에 한 명만 있는지 확인
    EXPECT_EQ(current_room->get_players()[0]->id, "player31"); // 새로 추가된 플레이어 확인
}

// 플레이어가 추가된 상태에서 타이머가 30ms 후 만료되는지 확인
TEST(RoomTest, TimerExpiresAfter30ms) {
    auto room = std::make_shared<Room>(1);

    bool expired = false;
    room->start_timer([&expired](int room_id, bool is_start) {
        expired = true;
    }, 30, 10); // 30ms 후 타이머 만료

    // 플레이어 추가
    auto player1 = std::make_shared<Player>("1", "Alice");
    room->add_player(player1);

    // 40ms 대기 후 타이머 만료 확인
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
    EXPECT_TRUE(expired);                  // 타이머가 만료되었는지 확인
    EXPECT_FALSE(room->is_timer_active()); // 타이머 비활성화 확인
}

// 플레이어가 모두 나가면 타이머가 즉시 만료되는지 확인
TEST(RoomTest, TimerExpiresWhenNoPlayers) {
    auto room = std::make_shared<Room>(1);

    bool expired = false;
    room->start_timer([&expired](int room_id, bool is_start) {
        expired = true;
    }, 30, 10); // 기본 30ms 대기, 10ms 간격으로 상태 확인

    // 플레이어 추가 후 제거
    auto player1 = std::make_shared<Player>("1", "Alice");
    room->add_player(player1);
    room->remove_player(player1);

    // 플레이어가 없는 상태에서 만료 확인
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
    EXPECT_TRUE(expired);                  // 타이머가 만료되었는지 확인
    EXPECT_FALSE(room->is_timer_active()); // 타이머 비활성화 확인
}

// 타이머가 강제 종료된 경우 만료 이벤트가 발생하지 않는지 확인
TEST(RoomTest, TimerStopsWhenForced) {
    auto room = std::make_shared<Room>(1);

    bool expired = false;
    room->start_timer([&expired](int room_id, bool is_start) {
        expired = true;
    }, 30, 10); // 기본 30ms 대기, 10ms 간격으로 상태 확인

    // 플레이어 추가 후 타이머 강제 종료
    auto player1 = std::make_shared<Player>("1", "Alice");
    room->add_player(player1);
    room->stop_timer();

    std::this_thread::sleep_for(std::chrono::milliseconds(40));

    EXPECT_FALSE(expired);                 // 만료 이벤트가 발생하지 않아야 함
    EXPECT_FALSE(room->is_timer_active()); // 타이머 비활성화 확인
}

// 플레이어가 추가되면 타이머가 다시 활성화되는지 확인
TEST(RoomTest, TimerResumesWhenPlayerAdded) {
    auto room = std::make_shared<Room>(1);

    bool expired = false;
    room->start_timer([&expired](int room_id, bool is_start) {
        expired = true;
    }, 30, 10); // 기본 30ms 대기, 10ms 간격으로 상태 확인

    // 플레이어 추가
    auto player1 = std::make_shared<Player>("1", "Alice");
    room->add_player(player1);
    EXPECT_TRUE(room->is_timer_active()); // 타이머 활성화 확인

    // 타이머 만료 전에 플레이어 제거
    room->remove_player(player1);
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
    EXPECT_FALSE(room->is_timer_active()); // 타이머 비활성화 확인
    EXPECT_TRUE(expired); // 만료 콜백 활성화 확인

    // 새로운 플레이어 추가
    auto player2 = std::make_shared<Player>("2", "Bob");
    EXPECT_FALSE(room->add_player(player2));
}