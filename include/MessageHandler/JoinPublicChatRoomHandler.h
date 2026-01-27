#pragma once

#include "MessageHandler.h"

class JoinPublicChatHandler : public MessageHandler{
    public:
        std::string handleMessage(ConnectionPtr conn, CommandPtr command, EpollInstancePtr epoll_instance = nullptr) override;
};

using JoinPublicChatHandlerPtr = std::shared_ptr<JoinPublicChatHandler>;