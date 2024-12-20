#ifndef EVENT_HPP
#define EVENT_HPP

#include <boost/asio.hpp>
#include <vector>
#include <memory>

using boost::asio::ip::tcp;

class Connection;

enum class EventType { READ, WRITE, CLOSE };

struct Event {
    EventType type;
    std::weak_ptr<Connection> connection; // 연결이 끊길수 있으므로, weak_ptr
    std::vector<char> data;
};

#endif // EVENT_HPP
