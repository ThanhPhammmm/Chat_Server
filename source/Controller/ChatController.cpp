#include "ChatController.h"

void ChatController::registerHandler(MessageHandlerPtr handler){
    handlers.push_back(handler);
}

std::string ChatController::handlerMessage(ConnectionPtr connection, std::string& message){
    CommandParser parser;
    auto cmd = parser.parse(message);
    
    for(auto& handler : handlers){
        if(handler->canHandle(cmd->type)){
            return handler->handleMessage(connection, cmd);
        }
    }
    
    return "Unknown command.";
}

