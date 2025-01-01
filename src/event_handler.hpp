#include <vector>
#include <memory>

class Connection;

class EventHandler {
public:
    static void handle_join_event(const std::vector<char>& data, const std::shared_ptr<Connection>& connection);
    static void handle_left_event(const std::vector<char>& data, const std::shared_ptr<Connection>& connection);
    static void handle_close_event(const std::vector<char>& data, const std::shared_ptr<Connection>& connection);
};