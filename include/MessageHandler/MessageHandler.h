#pragma once   

#include <Connection.h>
#include <Command.h>
#include <Epoll.h>

class MessageHandler{
    public:
        virtual ~MessageHandler() = default;
        virtual std::string handleMessage(ConnectionPtr conn, CommandPtr command, EpollInstancePtr epoll_instance = nullptr) = 0;
        virtual bool canHandle(CommandType type) = 0;
};

using MessageHandlerPtr = std::shared_ptr<MessageHandler>;