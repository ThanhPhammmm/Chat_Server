#include "PublicChatHandler.h"
#include "PublicChatRoom.h"
#include "UserManager.h"
#include "TimeUtils.h"

std::string PublicChatHandler::handleMessage(ConnectionPtr conn, CommandPtr command, EpollInstancePtr epoll_instance){
    (void)epoll_instance;

    if(!conn || conn->isClosed()){
        return "Error: Invalid connection";
    }
    
    int fd = conn->getFd();
    if(fd < 0){
        return "Error: Invalid file descriptor";
    }

    auto& userMgr = UserManager::getInstance();
    if(!userMgr.isLoggedIn(fd)){
        return "Error: Please login first. Use /login <username>";
    }

    auto& room = PublicChatRoom::getInstance(); 
    if(!room.isParticipant(fd)){
        return "Error: You must join public chat room first. Use /join_public_chat";
    }

    if(command->type == CommandType::LIST_USERS_IN_PUBLIC_CHAT_ROOM){
        std::string list = "Users in public chat:\n";
        int count = 0;
        for(auto& participant_fd : room.getParticipants()){
            auto username = userMgr.getUsername(participant_fd);
            if(username.has_value()){
                list += "  - " + username.value() + "\n";
                count++;
            }
        }
        if(count == 0) list += "  (none)\n";
        return list;
    }
    
    if(command->args.empty()){
        return "Error: No message provided";
    }
    
    std::string username = userMgr.getUsername(fd).value();
    std::string full_message = command->args[0];
    for(size_t i = 1; i < command->args.size(); i++){
        full_message += " " + command->args[i];
    }
    
    std::string timestamp = TimeUtils::getCurrentTimestamp();
    
    return "[" + timestamp + "] [Public] " + username + ": " + full_message;
}