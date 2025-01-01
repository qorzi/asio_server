#ifndef EVENT_HANDLER_HPP
#define EVENT_HANDLER_HPP

#include "event.hpp"
#include <stdexcept>
#include <memory>

/**
 * EventHandler
 * - 이벤트를 처리하는 정적 함수들
 */
class EventHandler {
public:
    // 각종 핸들러
    static void handle_join_event(const Event& event);
    static void handle_left_event(const Event& event);
    static void handle_close_event(const Event& event);

private:
    // 헬퍼: 반드시 커넥션이 있어야 하는 이벤트에 사용
    // 없거나 expired일 경우 runtime_error를 던짐
    static std::shared_ptr<Connection> get_required_connection(const Event& event, const char* context_name);
};

#endif // EVENT_HANDLER_HPP