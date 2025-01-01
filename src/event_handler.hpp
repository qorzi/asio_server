#include <vector>

class EventHandler {
public:
    static void handle_in_event(const std::vector<char>& data, const std::shared_ptr<Connection>& connection);
    static void handle_out_event(const std::vector<char>& data, const std::shared_ptr<Connection>& connection);
    static void handle_close_event(const std::vector<char>& data, const std::shared_ptr<Connection>& connection);
};