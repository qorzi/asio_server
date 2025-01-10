#include "game_event_handler.hpp"
#include "room.hpp"
#include "reactor.hpp"
#include "utils.hpp"
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
    case GameSubType::GAME_COUNTDOWN:
        handle_game_countdown(event);
        break;
    case GameSubType::GAME_START:
        handle_game_start(event);
        break;
    case GameSubType::PLAYER_MOVED:
        handle_player_moved(event);
        break;
    case GameSubType::PLAYER_COME_IN_MAP:
    case GameSubType::PLAYER_COME_OUT_MAP:
    case GameSubType::PLAYER_FINISHED:
        // 이벤트로 처리하지 않음 (통신 전용 이벤트)
        break;
    case GameSubType::GAME_END:
        // 게임 종료 및 게임 이력 저장 등등
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

    // 4) 플레이어 add (자동으로 첫 맵(A) 등에 배정)
    for (auto& p : players) {
        // 새 구조: Room::join_player() 이용
        room->join_player(p); 
    }

    // 5) Room 전체 정보 브로드캐스팅팅 (대기화면 이동 명령)
    {
        nlohmann::json broadcast_msg = room->extracte_all_map_info();
        broadcast_msg["action"] = "room_create";
        broadcast_msg["result"] = true;
        std::string body = broadcast_msg.dump();
        auto resp = Utils::create_response_string(MainEventType::GAME, (uint16_t)GameSubType::ROOM_CREATE, body);
        room->broadcast_message(resp);
    }

    // 6) 다음 이벤트: GAME_COUNTDOWN
    //    - room_id
    Event countEv;
    countEv.main_type = MainEventType::GAME;
    countEv.sub_type  = (uint16_t)GameSubType::GAME_COUNTDOWN;
    countEv.room_id   = rid;
    // 카운트다운 5초 (예: data="5")
    std::string countdown_val = "5"; 
    countEv.data.assign(countdown_val.begin(), countdown_val.end());

    Reactor::get_instance().enqueue_event(countEv);
}

/**
 * GAME_COUNTDOWN:
 * - ev.data에 남은 초 저장한다고 가정("5", "4", ...)
 * - 1초마다 broadcast, 남은초--, 0이면 GAME_START
 */
void GameEventHandler::handle_game_countdown(const Event& ev)
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
    {
        nlohmann::json broadcast_msg {
            {"action", "count_down"},
            {"result", true},
            {"count", str}
        };
        std::string body = broadcast_msg.dump();
        auto resp = Utils::create_response_string(MainEventType::GAME, (uint16_t)GameSubType::GAME_COUNTDOWN, body);
        room->broadcast_message(resp);
    }

    // 3) 남은 시간 확인 => 0이면 GAME_START
    if (remaining <= 0) {
        // GAME_START
        Event startEv;
        startEv.main_type = MainEventType::GAME;
        startEv.sub_type  = (uint16_t)GameSubType::GAME_START;
        startEv.room_id   = ev.room_id;
        Reactor::get_instance().enqueue_event(startEv);
        return;
    }

    // 4) 1초 후, 남은초-1 로 재귀 이벤트
    auto timer = std::make_shared<boost::asio::steady_timer>(ioc_);
    timer->expires_after(std::chrono::seconds(1));
    timer->async_wait([this, timer, room_id=ev.room_id, remaining](auto ec){
        if(!ec) {
            // 남은초 -1
            int nextSec = remaining - 1;
            Event next;
            next.main_type = MainEventType::GAME;
            next.sub_type  = (uint16_t)GameSubType::GAME_COUNTDOWN;
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
    // start broadcast
    {
        nlohmann::json broadcast_msg {
            {"action", "game_start"},
            {"result", true}
        };
        std::string body = broadcast_msg.dump();
        auto resp = Utils::create_response_string(MainEventType::GAME, (uint16_t)GameSubType::GAME_START, body);
        room->broadcast_message(resp);
    }
}

/**
 * PLAYER_MOVED:
 * - ev.data: JSON {"x":..., "y":...}
 * - 1) 플레이어 위치 검증(맵 범위, 이동 범위, 벽 등)
 * - 2) 이동
 * - 3) 같은 맵 플레이어에게만 이동 정보 브로드캐스트
 * - 4) 도착 확인
 */
void GameEventHandler::handle_player_moved(const Event& ev)
{
    auto player = ConnectionManager::get_instance().get_player_for_connection(ev.connection->lock());
    if(!player) {
        std::cerr << "[GameEventHandler] handle_player_moved: no player.\n";
        return;
    }

    auto room = game_manager_.find_room(player->room_id_);
    if(!room) {
        std::cerr << "[GameEventHandler] handle_player_moved: no romm.\n";
        return;
    }

    // parse pos
    std::string str(ev.data.begin(), ev.data.end());
    auto parsed = nlohmann::json::parse(str);
    int nx = parsed["x"].get<int>();
    int ny = parsed["y"].get<int>();

    // 현재 맵
    auto cur_map = player->current_map_.lock();
    if(!cur_map) {
        // 플레이어가 맵이 없다고? 이상 상황
        nlohmann::json broadcast_msg {
            {"error", "unknown"},
            {"result", false},
            {"message", "No current map for player"}
        };
        std::string body = broadcast_msg.dump();
        auto resp = Utils::create_response_string(MainEventType::ERROR, (uint16_t)ErrorSubType::UNKNOWN, body);
        player->send_message(resp);
        return;
    }

    // 1) 위치 검증: 범위 / 벽(추후) 
    Point newPos{nx, ny};
    std::cout << nx << " " << ny << '\n';
    if(!cur_map->is_valid_position(newPos) || !player->is_valid_position(newPos)) {
        // 응답: invalid pos
        nlohmann::json broadcast_msg {
            {"action", "player_moved"},
            {"result", false},
            {"player_id", player->id_},
            {"x", player->position_.x},
            {"y", player->position_.y},
            {"map", cur_map->name}
        };
        std::string body = broadcast_msg.dump();
        auto resp = Utils::create_response_string(MainEventType::ERROR, (uint16_t)ErrorSubType::UNKNOWN, body);
        player->send_message(resp);
        return;
    }

    // (벽 체크) => if(cur_map->is_blocked({nx,ny})) ...
    // ...

    // 2) 이동
    player->update_position(newPos);

    // 3) 해당 맵 broadcast
    {
        nlohmann::json broadcast_msg {
            {"action", "player_moved"},
            {"result", true},
            {"player_id", player->id_},
            {"x", nx},
            {"y", ny},
            {"map", cur_map->name}
        };
        std::string body = broadcast_msg.dump();
        auto resp = Utils::create_response_string(MainEventType::GAME, (uint16_t)GameSubType::PLAYER_MOVED, body);
        cur_map->broadcast_in_map(resp);
    }

    // 4) 도착인지 체크 (end_point)
    //    - 마지막 맵인 경우, end_point={299,299} etc.
    //    TODO: 모든 플레이어 도착 시, 게임 종료.
    if(cur_map->name == "C" // 마지막 맵
       && newPos == cur_map->end_point)
    {
        // 플레이어가 도착 => 게임 종료(또는 별도 rank)
        // remove from map, broadcast "finished"
        cur_map->remove_player(player);
        player->is_finished_ = true;

        // broadcast
        nlohmann::json broadcast_msg {
            {"action", "player_finished"},
            {"result", true},
            {"player_id", player->id_},
            {"total_dist", player->total_distance_}
        };
        std::string body = broadcast_msg.dump();
                auto resp = Utils::create_response_string(MainEventType::GAME, (uint16_t)GameSubType::PLAYER_FINISHED, body);
        room->broadcast_message(resp);

        // [TODO] 모든 플레이어 도착 시, 게임 종료 이벤트
        // 예: check if "room->allPlayersFinished()" => enqueue GAME_END, etc.
        // ...
        return;
    }

    // 5) 포탈인지 체크 => 다음 맵 이동
    if(cur_map->is_portal(newPos)) {

        // old map remove
        bool removed = cur_map->remove_player(player);
        if(removed) {
            nlohmann::json broadcast_msg {
                {"action", "player_come_out_map"},
                {"result", true},
                {"player_id", player->id_},
                {"map", cur_map->name}
            };
            std::string body = broadcast_msg.dump();
            auto resp = Utils::create_response_string(MainEventType::GAME, (uint16_t)GameSubType::PLAYER_COME_OUT_MAP, body);
            cur_map->broadcast_in_map(resp);
        }

        // 포탈의 linked_map_name 찾기
        std::string linked_map="";
        for(auto& pt : cur_map->portals_) {
            if(pt.position == player->position_) {
                linked_map = pt.linked_map_name;
                break;
            }
        }
        if(!linked_map.empty()) {
            auto new_map = room->get_map_by_name(linked_map);
            if(new_map) {
                new_map->add_player(player);
                player->current_map_ = new_map;
                // 새 맵의 start_point로 위치를 업데이트
                player->update_position(new_map->start_point);

                // broadcast
                nlohmann::json broadcast_msg {
                    {"action","player_come_in_map"},
                    {"result", true},
                    {"player_id",player->id_},
                    {"map", new_map->name},
                    {"x", player->position_.x},
                    {"y", player->position_.y}
                };
                auto body = broadcast_msg.dump();
                auto resp = Utils::create_response_string(MainEventType::GAME, (uint16_t)GameSubType::PLAYER_COME_IN_MAP, body);
                new_map->broadcast_in_map(resp);
            } else {
                // rollback?
                cur_map->add_player(player);
                player->current_map_ = cur_map;

                //broadcast
                nlohmann::json broadcast_msg {
                    {"error", "unknown"},
                    {"result", false},
                    {"message", "Portal leads to unknown map."}
                };
                std::string body = broadcast_msg.dump();
                auto resp = Utils::create_response_string(MainEventType::ERROR, (uint16_t)ErrorSubType::UNKNOWN, body);
                player->send_message(resp);
            }
        }
    }
}