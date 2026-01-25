#include "PublicChatHandler.h"

bool PublicChatHandler::canHandle(CommandType type){
    return type == CommandType::PUBLIC_CHAT;
}

std::string PublicChatHandler::handleMessage(ConnectionPtr conn, CommandPtr command, EpollPtr epoll_instance){
    if(command->args.empty()){
        return "Error: No message provided for public chat.";
    }

    std::string full_message = command->args[0];
    for(size_t i = 1; i < command->args.size(); i++){
        full_message += " " + command->args[i];
    }

    std::string chat_message = "[Public] fd=" 
                              + std::to_string(conn->getFd()) 
                              + ": " 
                              + full_message;
    
    return chat_message;
}