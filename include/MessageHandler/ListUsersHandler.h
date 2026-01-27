#pragma once

#include "MessageHandler.h"

class ListUsersHandler : public MessageHandler{
    public:
        std::string handleMessage(ConnectionPtr conn, CommandPtr command, EpollInstancePtr epoll_instance) override;
};

using ListUsersHandlerPtr = std::shared_ptr<ListUsersHandler>;