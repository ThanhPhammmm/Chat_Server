#pragma once

#include "MessageHandler.h"

class PublicChatHandler : public MessageHandler{
    public:
        std::string handleMessage(ConnectionPtr conn, CommandPtr command, EpollPtr epoll_instance = nullptr) override;
        bool canHandle(CommandType type) override;
};

using PublicChatHandlerPtr = std::shared_ptr<PublicChatHandler>;