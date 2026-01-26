#pragma once

#include "MessageHandler.h"

class LeavePublicChatHandler : public MessageHandler{
    public:
        std::string handleMessage(ConnectionPtr conn, CommandPtr command, EpollInstancePtr epoll_instance = nullptr) override;
        bool canHandle(CommandType type) override;
};

using LeavePublicChatHandlerPtr = std::shared_ptr<LeavePublicChatHandler>;