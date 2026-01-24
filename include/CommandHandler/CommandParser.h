#pragma once

#include "Command.h"

class CommandParser {
    public:
        CommandPtr parse(std::string& raw_message);
};