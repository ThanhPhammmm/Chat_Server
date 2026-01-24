#include "PublicChatHandler.h"

bool PublicChatHandler::canHandle(CommandType type) {
    return type == CommandType::PUBLIC_CHAT;
}

std::string PublicChatHandler::handleMessage(ConnectionPtr conn, CommandPtr command) {
    if(command->args.empty()){
        return "Error: No message provided for public chat.";
    }

    std::string chat_message = "[Public] " + std::to_string(conn->getFd()) + ": " + command->args[0];
    return chat_message;
}