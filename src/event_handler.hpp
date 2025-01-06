#ifndef EVENT_HANDLER_HPP
#define EVENT_HANDLER_HPP

#include "event.hpp"
#include "game_manager.hpp"
#include <stdexcept>
#include <memory>

/**
 * EventHandler
 * - 이벤트를 처리하는 정적 함수들
 */
class EventHandler {
public:
    // 정적 초기화
    static void init(GameManager* gm);
    // 각종 핸들러
    static void handle_join_event(const Event& event);
    static void handle_left_event(const Event& event);
    static void handle_close_event(const Event& event);

private:
    static GameManager* game_manager_;
    // 헬퍼: 반드시 커넥션이 있어야 하는 이벤트에 사용
    // 없거나 expired일 경우 runtime_error를 던짐
    static std::shared_ptr<Connection> get_required_connection(const Event& event, const char* context_name);

    // 헬퍼: game_manager_ 포인터가 유효한지 확인
    static void check_game_manager_or_throw(const char* func_name);
};

#endif // EVENT_HANDLER_HPP