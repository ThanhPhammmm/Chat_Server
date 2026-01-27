#include "PublicChatHandler.h"
#include "PublicChatRoom.h"

std::string PublicChatHandler::handleMessage(ConnectionPtr conn, CommandPtr command, EpollInstancePtr epoll_instance){
    if(!conn || conn->isClosed()){
        return "Error: Invalid connection";
    }
    
    int fd = conn->getFd();
    if(fd < 0){
        return "Error: Invalid file descriptor";
    }

    PublicChatRoom tmp;
    auto& room = tmp.getInstance();
    
    if(!room.isParticipant(fd)){
        return "Error: You must join public chat room first. Use /join_public_chat";
    }

    if((command->args.empty()) && (command->type != CommandType::LIST_USERS_IN_PUBLIC_CHAT_ROOM)){
        return "Error: No message provided for public chat.";
    }

    std::string chat_message = "";
    int count = 0;
    
    if(command->type == CommandType::LIST_USERS_IN_PUBLIC_CHAT_ROOM){
        for(auto& e: room.getParticipants()){
            if(count > 0) chat_message += ", ";
            chat_message += std::to_string(e);
            count++;
        }
    }
    else if(command->type == CommandType::PUBLIC_CHAT){
        std::string full_message = command->args[0];
        for(size_t i = 1; i < command->args.size(); i++){
            full_message += " " + command->args[i];
        }

        chat_message = "[Public] fd=" 
                                + std::to_string(conn->getFd()) 
                                + ": " 
                                + full_message;
    }
    return chat_message;
}