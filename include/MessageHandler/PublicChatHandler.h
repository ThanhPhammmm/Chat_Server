#pragma once

#include "MessageHandler.h"

class PublicChatHandler : public MessageHandler{
    public:
        std::string handleMessage(ConnectionPtr conn, CommandPtr command, EpollInstancePtr epoll_instance = nullptr) override;
};

using PublicChatHandlerPtr = std::shared_ptr<PublicChatHandler>;