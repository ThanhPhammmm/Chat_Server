#pragma once

#include "CommandParser.h"
#include "MessageHandler.h"
#include "Connection.h"

class ChatController{
    private:
        std::vector<MessageHandlerPtr> handlers;

    public:
        void registerHandler(MessageHandlerPtr handler);
        std::string handlerMessage(ConnectionPtr connection, std::string& message);
};

using ChatControllerPtr = std::shared_ptr<ChatController>;
