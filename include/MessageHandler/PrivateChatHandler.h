#pragma once

#include "MessageHandler.h"

class PrivateChatHandler : public MessageHandler{
    public:
        std::string handleMessage(ConnectionPtr conn, CommandPtr command, EpollInstancePtr epoll_instance) override;
};

using PrivateChatHandlerPtr = std::shared_ptr<PrivateChatHandler>;