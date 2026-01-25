#pragma once

#include "MessageHandler.h"

class ListUsersHandler : public MessageHandler{
    public:
        std::string handleMessage(ConnectionPtr conn, CommandPtr command, EpollPtr epoll_instance);
        bool canHandle(CommandType type) override;
};

using ListUsersHandlerPtr = std::shared_ptr<ListUsersHandler>;