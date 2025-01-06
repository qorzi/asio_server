#include "event_handler.hpp"
#include "connection.hpp"
#include "connection_manager.hpp"
#include "header.hpp"
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

GameManager* EventHandler::game_manager_ = nullptr;

// GameManager 주입
void EventHandler::init(GameManager* gm) {
    game_manager_ = gm;
    if (!game_manager_) {
        std::cerr << "[WARNING] EventHandler::init called with nullptr.\n";
    } else {
        std::cout << "[INFO] EventHandler::init - GameManager set.\n";
    }
}

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


void EventHandler::check_game_manager_or_throw(const char* func_name) {
    if (!game_manager_) {
        throw std::runtime_error(std::string("[ERROR] GameManager is not set. Call EventHandler::init() before using ") + func_name);
    }
}

// 헬퍼: 응답을 헤더+바디 형태로 직렬화
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

/** 
 * JOIN 이벤트 처리
 *  1) JSON 파싱 -> player_id, player_name
 *  2) Player 생성
 *  3) ConnectionManager 에 "플레이어-커넥션" 등록
 *  4) GameManager::join_game(player) 호출 -> 내부적으로 대기열 or Room 배정
 */
void EventHandler::handle_join_event(const Event& event)
{
    // GameManager 유효성 체크
    check_game_manager_or_throw("handle_join_event");

    // 커넥션 필요
    auto connection = get_required_connection(event, "JOIN");

    try {
        // 1) JSON 파싱
        json parsed_data = json::parse(event.data);
        std::string player_id   = parsed_data["player_id"].get<std::string>();
        std::string player_name = parsed_data["player_name"].get<std::string>();

        // 2) Player 생성
        auto player = std::make_shared<Player>(player_id, player_name);

        // 3) ConnectionManager 등록
        ConnectionManager::get_instance().register_connection(player, connection);

        // 4) GameManager에 join_game()
        game_manager_->join_game(player);
    } 
    catch (const std::exception& e) {
        std::cerr << "[ERROR] handle_join_event: " << e.what() << std::endl;
    }
}

/**
 * LEFT 이벤트 처리
 *  1) 커넥션 -> 플레이어 찾기
 *  2) GameManager::remove_player_from_room(player)
 */
void EventHandler::handle_left_event(const Event& event)
{
    check_game_manager_or_throw("handle_left_event");

    auto connection = get_required_connection(event, "LEFT");

    try {
        auto player = ConnectionManager::get_instance().get_player_for_connection(connection);
        if (!player) {
            std::cerr << "[WARNING] No player associated with this connection.\n";
            return;
        }

        // remove_player_from_room -> 내부적으로 대기열이면 대기열에서 제거, 룸이면 룸에서 제거
        game_manager_->remove_player_from_room(player);

        // 필요하면 cm.unregister_connection(player);
    } 
    catch (const std::exception& e) {
        std::cerr << "[ERROR] handle_left_event: " << e.what() << std::endl;
    }
}

/**
 * CLOSE 이벤트 처리
 *  - 소켓이 끊어졌으므로, 플레이어가 방에 있으면 제거
 *  - 커넥션 매니저도 정리
 *  - 소켓 close
 */
void EventHandler::handle_close_event(const Event& event)
{
    check_game_manager_or_throw("handle_close_event");

    auto connection = get_required_connection(event, "CLOSE");

    try {
        auto player = ConnectionManager::get_instance().get_player_for_connection(connection);
        if (player) {
            // 방(또는 대기열)에서 제거
            game_manager_->remove_player_from_room(player);

            // 커넥션 매니저에서 해제
            ConnectionManager::get_instance().unregister_connection(player);
        }
        else {
            std::cerr << "[WARNING] No player found for this connection.\n";
        }

        // 소켓 닫기
        if (connection->get_socket().is_open()) {
            boost::system::error_code ec;
            connection->get_socket().close(ec);
            if (ec) {
                std::cerr << "[ERROR] Failed to close socket: " << ec.message() << std::endl;
            } else {
                std::cout << "[INFO] Socket closed by handle_close_event.\n";
            }
        }
    } 
    catch (const std::exception& e) {
        std::cerr << "[ERROR] handle_close_event: " << e.what() << std::endl;
    }
}