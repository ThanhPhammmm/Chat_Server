#pragma once

#include "MessageHandler.h"

class LogoutChatHandler : public MessageHandler{
    public:
        std::string handleMessage(ConnectionPtr conn, CommandPtr command, EpollInstancePtr epoll_instance = nullptr) override;
};

using LogoutChatHandlerPtr = std::shared_ptr<LogoutChatHandler>;