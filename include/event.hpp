#ifndef EVENT_HPP
#define EVENT_HPP

#include <boost/asio.hpp>
#include <vector>
#include <memory>

using boost::asio::ip::tcp;

class Connection;

enum class EventType { READ, WRITE, CLOSE, ERROR };

struct Event {
    EventType type;
    std::weak_ptr<Connection> connection; // 연결이 끊길수 있으므로, weak_ptr
    std::vector<char> data; // data 또는 json 바디
};

#endif // EVENT_HPP
