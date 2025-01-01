#include "event_handler.hpp"
#include "connection.hpp"
#include "server.hpp"
#include "connection_manager.hpp"
#include "header.hpp"
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

// 헬퍼 함수 구현
std::shared_ptr<Connection> EventHandler::get_required_connection(const Event& event, const char* context_name)
{
    // 1) optional 인지, has_value() 확인
    if (!event.connection.has_value()) {
        throw std::runtime_error(std::string("[ERROR] No connection in ") + context_name + " event");
    }

    // 2) weak_ptr -> lock()
    auto conn = event.connection.value().lock();
    if (!conn) {
        throw std::runtime_error(std::string("[ERROR] Connection expired in ") + context_name + " event");
    }

    return conn;
}

static std::string create_response_string(RequestType type, const std::string& body) {
    // 헤더 생성
    Header header{type, static_cast<uint32_t>(body.size())};

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

// JOIN 이벤트 처리
void EventHandler::handle_join_event(const Event& event)
{
    // 반드시 커넥션이 필요한 이벤트
    auto connection = get_required_connection(event, "JOIN");

    try {
        json parsed_data = json::parse(event.data);
        std::string player_id   = parsed_data["player_id"].get<std::string>();
        std::string player_name = parsed_data["player_name"].get<std::string>();

        auto player = std::make_shared<Player>(player_id, player_name);

        // 서버 싱글턴
        Server& server = Server::getInstance();
        server.add_player_to_room(player);

        auto room = player->room.lock(); // 이제 Player->room이 weak_ptr이면 lock()
        if (room) {
            json room_info;
            room_info["room_id"] = room->id;
            room_info["players"] = json::array();
            for (const auto& p : room->get_players()) {
                room_info["players"].push_back({
                    {"id", p->id},
                    {"name", p->name}
                });
            }
            std::string response = create_response_string(RequestType::JOIN, room_info.dump());
            room->broadcast_message(response);
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] handle_join_event: " << e.what() << std::endl;
    }
}

// LEFT 이벤트 처리
void EventHandler::handle_left_event(const Event& event)
{
    auto connection = get_required_connection(event, "LEFT");

    try {
        Server& server = Server::getInstance();
        ConnectionManager& cm = server.get_connection_manager();

        auto player = cm.get_player_for_connection(connection);
        if (!player) {
            std::cerr << "[WARNING] No player associated with this connection.\n";
            return;
        }

        auto room = player->room.lock();
        if (room) {
            bool removed = room->remove_player(player);
            if (removed) {
                json room_info;
                room_info["room_id"] = room->id;
                room_info["players"] = json::array();

                for (const auto& p : room->get_players()) {
                    room_info["players"].push_back({
                        {"id", p->id},
                        {"name", p->name}
                    });
                }

                std::string response = create_response_string(RequestType::LEFT, room_info.dump());
                room->broadcast_message(response);
            } else {
                std::cout << "[WARNING] Player was already removed from room.\n";
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] handle_left_event: " << e.what() << std::endl;
    }
}

// CLOSE 이벤트 처리
void EventHandler::handle_close_event(const Event& event)
{
    auto connection = get_required_connection(event, "CLOSE");

    try {
        // (1) 기존 handle_out_event 로직과 동일하게 "방에서 플레이어 제거" + 브로드캐스트
        Server& server = Server::getInstance();
        ConnectionManager& cm = server.get_connection_manager();

        auto player = cm.get_player_for_connection(connection);
        if (!player) {
            // 이미 해제된 플레이어일 수도
            std::cerr << "[WARNING] No player found for this connection.\n";
        } else {
            auto room = player->room.lock();
            if (room) {
                bool removed = room->remove_player(player);
                if (removed) {
                    json room_info;
                    room_info["room_id"] = room->id;
                    room_info["players"] = json::array();

                    for (const auto& p : room->get_players()) {
                        room_info["players"].push_back({
                            {"id", p->id},
                            {"name", p->name}
                        });
                    }

                    std::string response = create_response_string(RequestType::LEFT, room_info.dump());
                    room->broadcast_message(response);
                }
            }
        }

        // (2) 소켓 닫기 로직 추가
        if (connection->get_socket().is_open()) {
            boost::system::error_code ec;
            connection->get_socket().close(ec);
            if (ec) {
                std::cerr << "[ERROR] Failed to close socket: " << ec.message() << std::endl;
            } else {
                std::cout << "[INFO] Socket closed by handle_close_event.\n";
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] handle_close_event: " << e.what() << std::endl;
    }
}
