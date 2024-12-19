#include "parser.hpp"
#include <stdexcept>

Header Parser::parse_header(const std::vector<char>& header) {
    if (header.size() < 5) {
        throw std::invalid_argument("Header size is too small");
    }

    Header parsed_header;
    parsed_header.type = static_cast<RequestType>(header[0]);
    parsed_header.body_length = (static_cast<uint32_t>(header[1]) << 24) |
                                (static_cast<uint32_t>(header[2]) << 16) |
                                (static_cast<uint32_t>(header[3]) << 8) |
                                static_cast<uint32_t>(header[4]);

    return parsed_header;
}

std::string Parser::process_body(const std::string& body) {
    return "Task processed: " + body;
}