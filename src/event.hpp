#ifndef EVENT_HPP
#define EVENT_HPP

#include <boost/asio.hpp>
#include <vector>
#include <memory>
#include <optional>

#include "header.hpp"

using boost::asio::ip::tcp;

class Connection;

enum class EventType { REQUEST, WRITE, CLOSE, ERROR };

struct Event {
    EventType type;
    std::optional<std::weak_ptr<Connection>> connection; // 이벤트 종류에 따라 커넥션이 없을 수 있음 ( 서버 자체 이벤트 )
    RequestType request_type = RequestType::UNKNOWN; // 리퀘스트 타입 (REQUEST 이벤트에서만 사용)
    std::vector<char> data; // data 또는 json 바디
};

#endif // EVENT_HPP
