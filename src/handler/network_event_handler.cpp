#include "network_event_handler.hpp"
#include "connection.hpp"
#include "reactor.hpp"
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

NetworkEventHandler::NetworkEventHandler(GameManager& gm)
    : game_manager_(gm)
{
}

void NetworkEventHandler::handle_event(const Event& event)
{
    if (event.main_type != MainEventType::NETWORK) {
        std::cerr << "[WARNING] NetworkEventHandler received non-NETWORK event.\n";
        return;
    }

    switch (static_cast<NetworkSubType>(event.sub_type)) {
    case NetworkSubType::JOIN:
        handle_join(event);
        break;
    case NetworkSubType::LEFT:
        handle_left(event);
        break;
    case NetworkSubType::CLOSE:
        handle_close(event);
        break;
    default:
        std::cerr << "[WARNING] Unknown NetworkSubType: " << event.sub_type << std::endl;
        break;
    }
}

std::shared_ptr<Connection> NetworkEventHandler::get_required_connection(const Event& event, const char* context_name)
{
    if (!event.connection.has_value()) {
        throw std::runtime_error(std::string("[ERROR] No connection in ") + context_name + " event");
    }
    auto conn = event.connection.value().lock();
    if (!conn) {
        throw std::runtime_error(std::string("[ERROR] Connection expired in ") + context_name + " event");
    }
    return conn;
}

// JOIN: 대기열 등록
// DATA: 클라이언트에서 "player_id, player_name" JSON 파라미터
void NetworkEventHandler::handle_join(const Event& event)
{
    try {
        auto conn = get_required_connection(event, "JOIN");

        // 1) JSON 파싱
        json parsed = json::parse(event.data);
        std::string player_id   = parsed["player_id"].get<std::string>();
        std::string player_name = parsed["player_name"].get<std::string>();

        // 2) Player 생성
        auto player = std::make_shared<Player>(player_id, player_name);

        // 3) ConnectionManager에 연결 등록
        ConnectionManager::get_instance().register_connection(player, conn);

        // 4) 대기열에 추가
        game_manager_.add_waiting_player(player);

        // 5) 여기서는 간단히 join 완료 알림
        std::string ack = R"({"result":"ok","action":"join"})";
        conn->async_write(ack);

        // 6) 인원 수 확인 -> 5명 이상이면 ROOM_CREATE 이벤트
        size_t waiting_count = game_manager_.waiting_count();
        if (waiting_count >= 5) {
            // 이벤트 enqueue: ROOM_CREATE
            Event ev;
            ev.main_type = MainEventType::GAME;
            ev.sub_type = (uint16_t)GameSubType::ROOM_CREATE;
            Reactor::get_instance().enqueue_event(ev);
        }

    } catch (std::exception& e) {
        std::cerr << "[ERROR] handle_join: " << e.what() << "\n";
    }
}

// LEFT: 대기열 제거만 수행
// DATA: 클라이언트에서 "player_id, player_name" JSON 파라미터 (받지만 안씀)
void NetworkEventHandler::handle_left(const Event& event)
{
    try {
        auto conn = get_required_connection(event, "LEFT");
        auto player = ConnectionManager::get_instance().get_player_for_connection(conn);
        if (!player) {
            std::cerr << "[WARNING] handle_left: No player for this connection.\n";
            return;
        }

        // 대기열에서 제거
        bool removed = game_manager_.remove_waiting_player(player);
        if (removed) {
            conn->async_write(R"({"result":"ok","action":"left","msg":"removed from waiting list"})");
        } else {
            conn->async_write(R"({"result":"not_found","action":"left","msg":"player not in waiting list"})");
        }

    } catch (std::exception& e) {
        std::cerr << "[ERROR] handle_left: " << e.what() << "\n";
    }
}

void NetworkEventHandler::handle_close(const Event& event)
{
    try {
        auto conn = get_required_connection(event, "CLOSE");
        auto player = ConnectionManager::get_instance().get_player_for_connection(conn);
        if (player) {
            game_manager_.remove_waiting_player(player);
            ConnectionManager::get_instance().unregister_connection(player);
        }
        // 소켓 닫기
        if (conn->get_socket().is_open()) {
            boost::system::error_code ec;
            conn->get_socket().close(ec);
            if (ec) {
                std::cerr << "[ERROR] handle_close: " << ec.message() << "\n";
            }
        }
    } catch (std::exception& e) {
        std::cerr << "[ERROR] handle_close: " << e.what() << "\n";
    }
}
