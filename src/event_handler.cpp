#include "event_handler.hpp"
#include "header.hpp"
#include "server.hpp"
#include "connection.hpp"

#include <string>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::vector<char> serialize_header(const Header& header) {
    std::vector<char> buffer(8);
    std::memcpy(buffer.data(), &header, sizeof(Header));
    std::cout << "[DEBUG] Serialized Header: ";
    for (char c : buffer) {
        std::cout << std::hex << static_cast<int>(c & 0xFF) << " ";
    }
    std::cout << std::endl;
    return buffer;
}

std::string create_response_string(RequestType type, const std::string& body) {
    // 헤더 생성
    Header header{type, static_cast<uint32_t>(body.size())};

    std::cout << "-----Size of Header: " << sizeof(Header) << std::endl;
    serialize_header(header);

    // 헤더를 바이너리 데이터로 변환
    std::vector<char> header_buffer(sizeof(Header));
    std::memcpy(header_buffer.data(), &header, sizeof(Header));

    // 본문 데이터 패딩 처리 (8바이트 배수로 맞춤)
    size_t padded_body_length = ((body.size() + 7) / 8) * 8; // 8의 배수로 맞춤
    std::string padded_body = body;
    padded_body.resize(padded_body_length, '\0'); // '\0'으로 패딩 추가

    // 헤더와 패딩된 본문 결합
    std::string response(header_buffer.begin(), header_buffer.end());
    response += padded_body;

    return response; // 결합된 헤더 + 패딩된 본문 스트링 반환
}

// IN 이벤트 처리
void EventHandler::handle_in_event(const std::vector<char>& data, const std::shared_ptr<Connection>& connection) {
    try {
        json parsed_data = json::parse(data);
        std::string player_id = parsed_data["player_id"].get<std::string>();
        std::string player_name = parsed_data["player_name"].get<std::string>();

        auto player = std::make_shared<Player>(player_id, player_name);
        Server& server = Server::getInstance();
        server.add_player_to_room(player);

        auto room = player->room;
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

            std::string response = create_response_string(RequestType::IN, room_info.dump());
            room->broadcast_message(response);
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Failed to handle IN event: " << e.what() << std::endl;
    }
}

// OUT 이벤트 처리
void EventHandler::handle_out_event(const std::vector<char>& data, const std::shared_ptr<Connection>& connection) {
    try {
        Server& server = Server::getInstance();
        ConnectionManager& connection_manager = server.get_connection_manager();

        auto player = connection_manager.get_player_for_connection(connection);
        auto room = player->room;

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

                std::string response = create_response_string(RequestType::OUT, room_info.dump());
                room->broadcast_message(response);
            } else {
                std::cout << "[WARNING] Player already removed from room.\n";
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Failed to handle OUT event: " << e.what() << std::endl;
    }
}
