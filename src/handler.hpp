#pragma oncee

namespace redis {

enum class Command {
    GET,
    SET,
    DEL,
    PING,
    UNKNOWN
};

Command parse_command(const std::string& str);

}