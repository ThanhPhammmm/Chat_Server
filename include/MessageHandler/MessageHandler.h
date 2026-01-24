#pragma once   

#include "Command.h"
#include "Connection.h"

class MessageHandler{
    public:
        virtual ~MessageHandler() = default;
        virtual std::string handleMessage(ConnectionPtr conn, CommandPtr command) = 0;
        virtual bool canHandle(CommandType type) = 0;
};

using MessageHandlerPtr = std::shared_ptr<MessageHandler>;