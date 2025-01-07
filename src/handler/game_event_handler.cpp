#include "game_event_handler.hpp"
#include "room.hpp"
#include "reactor.hpp"
#include <nlohmann/json.hpp>
#include <iostream>

GameEventHandler::GameEventHandler(GameManager& gm, boost::asio::io_context& ioc)
    : game_manager_(gm)
    , ioc_(ioc)
{
}

void GameEventHandler::handle_event(const Event& event)
{
    if (event.main_type != MainEventType::GAME) {
        std::cerr << "[WARNING] GameEventHandler got non-GAME event.\n";
        return;
    }

    switch (static_cast<GameSubType>(event.sub_type)) {
    case GameSubType::ROOM_CREATE:
        handle_room_create(event);
        break;
    case GameSubType::GAME_START_COUNTDOWN:
        handle_game_start_countdown(event);
        break;
    case GameSubType::GAME_START:
        handle_game_start(event);
        break;
    case GameSubType::PLAYER_MOVED:
        handle_player_moved(event);
        break;
    default:
        std::cerr << "[GameEventHandler] Unknown sub_type=" << event.sub_type << "\n";
    }
}

/** 
 * ROOM_CREATE:
 * - 대기열(5명) pop 해서 방 만들고
 * - 방에 플레이어 추가 + broadcast("go to waiting screen")
 * - 다음 이벤트로 "GAME_START_COUNTDOWN" enqueue
 */
void GameEventHandler::handle_room_create(const Event& ev)
{
    // 1) 5명 pop
    auto players = game_manager_.pop_waiting_players(5);
    if (players.empty()) {
        std::cout << "[GameEventHandler] handle_room_create but no players.\n";
        return;
    }
    // 2) room_id
    int rid = game_manager_.current_room_id++;
    auto room = game_manager_.create_room(rid);

    // 3) 맵 초기화
    room->initialize_maps();

    // 4) 플레이어 add
    for (auto& p : players) {
        room->add_player(p);
        // 대기화면 이동 명령
        p->send_message(R"({"action":"waiting_screen","room_id":)" + std::to_string(rid) + "}");
    }
    room->broadcast_message("[ROOM] Created room " + std::to_string(rid) + ", going to waiting screen...");

    // 5) 다음 이벤트: GAME_START_COUNTDOWN
    //   - room_id
    Event countEv;
    countEv.main_type = MainEventType::GAME;
    countEv.sub_type  = (uint16_t)GameSubType::GAME_START_COUNTDOWN;
    countEv.room_id   = rid;
    // 카운트다운 5초 (가령 data[0]에 저장 or JSON)
    // 여기서는 예시로 data에 "5" ASCII
    std::string countdown_val = "5"; 
    countEv.data.assign(countdown_val.begin(), countdown_val.end());

    Reactor::get_instance().enqueue_event(countEv);
}

/**
 * GAME_START_COUNTDOWN:
 * - ev.data에 남은 초 저장한다고 가정("5", "4", ...)
 * - 1초마다 broadcast, 남은초--, 0이면 GAME_START
 */
void GameEventHandler::handle_game_start_countdown(const Event& ev)
{
    auto room = game_manager_.find_room(ev.room_id);
    if (!room) {
        std::cerr << "[GameEventHandler] handle_game_start_countdown: no room found.\n";
        return;
    }

    // 1) parse 남은초
    std::string str(ev.data.begin(), ev.data.end());
    int remaining = std::stoi(str);

    // 2) 카운트다운 브로드캐스트
    room->broadcast_message("[COUNTDOWN] " + std::to_string(remaining) + " seconds left...");

    // 3) 남은 시간 확인 및 게임 시작 이벤트
    if (remaining <= 0) {
        // GAME_START
        Event startEv;
        startEv.main_type = MainEventType::GAME;
        startEv.sub_type  = (uint16_t)GameSubType::GAME_START;
        startEv.room_id   = ev.room_id;
        Reactor::get_instance().enqueue_event(startEv);
        return;
    }

    // 4) 1초 타이머 감산
    auto timer = std::make_shared<boost::asio::steady_timer>(ioc_);
    timer->expires_after(std::chrono::seconds(1));
    timer->async_wait([this, timer, room_id=ev.room_id, remaining](auto ec){
        if(!ec) {
            // 남은초 -1
            int nextSec = remaining - 1;
            Event next;
            next.main_type = MainEventType::GAME;
            next.sub_type  = (uint16_t)GameSubType::GAME_START_COUNTDOWN;
            next.room_id   = room_id;
            auto s = std::to_string(nextSec);
            next.data.assign(s.begin(), s.end());
            Reactor::get_instance().enqueue_event(next);
        }
    });
}

/** 
 * GAME_START:
 * - 실제 게임 시작
 * - room->broadcast("Game Start!")
 * - room->start_game() or etc.
 */
void GameEventHandler::handle_game_start(const Event& ev)
{
    auto room = game_manager_.find_room(ev.room_id);
    if(!room) {
        std::cerr << "[GameEventHandler] handle_game_start: no room.\n";
        return;
    }
    room->broadcast_message("[GAME] Start now!");
}

/**
 * PLAYER_MOVED:
 * - ev.data: JSON {"x":..., "y":...}
 * - update player position
 * - broadcast to all
 */
void GameEventHandler::handle_player_moved(const Event& ev)
{
    auto room = game_manager_.find_room(ev.room_id);
    if(!room) return;
    auto player = room->find_player(ev.player_id);
    if(!player) return;

    // parse pos
    // ex) JSON
    std::string str(ev.data.begin(), ev.data.end());
    auto parsed = nlohmann::json::parse(str);
    int nx = parsed["x"].get<int>();
    int ny = parsed["y"].get<int>();

    player->update_position({nx, ny});
    room->broadcast_message("[ROOM] Player " + ev.player_id + " moved to (" 
                            + std::to_string(nx) + "," + std::to_string(ny) + ")");
}