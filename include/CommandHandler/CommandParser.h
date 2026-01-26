#pragma once

#include "Command.h"
#include "Message.h"
#include "Epoll.h"

class CommandParser{
    public:
        CommandPtr parse(std::string& raw_message, IncomingMessage& incomming, EpollInstancePtr epoll_instance);
};