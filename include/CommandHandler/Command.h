#pragma once

#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <stdint.h>

enum class CommandType{
    LOGIN,
    LOGOUT,
    PUBLIC_CHAT,
    PRIVATE_CHAT,
    LIST_USERS,
    JOIN_PUBLIC_CHAT_ROOM,
    LEAVE_PUBLIC_CHAT_ROOM,
    UNKNOWN
};

class Command{
    public:
        CommandType type;
        std::vector<std::string> args;
        std::string raw_message;
        Command(CommandType t = CommandType::UNKNOWN);
};

using CommandPtr = std::shared_ptr<Command>;
