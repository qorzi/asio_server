#ifndef GAME_EVENT_HANDLER_HPP
#define GAME_EVENT_HANDLER_HPP

#include "event.hpp"
#include "game_manager.hpp"

class GameEventHandler {
public:
    // 생성자에서 GameManager를 받아서 저장 (DI)
    explicit GameEventHandler(GameManager& gm, boost::asio::io_context& ioc);

    // 이벤트 처리 함수
    void handle_event(const Event& event);

private:
    GameManager& game_manager_;
    boost::asio::io_context& ioc_;

    // 서브 핸들러들
    void handle_room_create(const Event& ev);
    void handle_game_start_countdown(const Event& ev);
    void handle_game_start(const Event& ev);
    void handle_player_moved(const Event& ev);
};

#endif // GAME_EVENT_HANDLER_HPP
