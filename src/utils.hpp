#ifndef UTILS_HPP
#define UTILS_HPP

#include "event.hpp"
#include <string>


namespace Utils {
    std::string create_response_string(MainEventType main_type, uint16_t sub_type, const std::string& body);
}

#endif // UTILS_HPP
