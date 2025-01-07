#ifndef NETWORK_EVENT_HANDLER_HPP
#define NETWORK_EVENT_HANDLER_HPP

#include "event.hpp"
#include "game_manager.hpp"
#include "connection_manager.hpp"

class NetworkEventHandler {
public:
    // 생성자에서 GameManager를 받아서 저장 (DI)
    explicit NetworkEventHandler(GameManager& gm);

    // 이벤트 처리 함수
    void handle_event(const Event& event);

private:
    GameManager& game_manager_;

    // 서브 핸들러들
    void handle_join(const Event& event);
    void handle_left(const Event& event);
    void handle_close(const Event& event);

    // 유틸
    std::shared_ptr<Connection> get_required_connection(const Event& event, const char* context_name);
};

#endif // NETWORK_EVENT_HANDLER_HPP
