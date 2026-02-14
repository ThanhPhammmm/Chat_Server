#include "PrivateChatHandler.h"
#include "PrivateChatThreadHandler.h"
#include "PublicChatRoom.h"
#include "UserManager.h"
#include "TimeUtils.h"
#include "MessageUtils.h"

std::string PrivateChatHandler::handleMessage(ConnectionPtr conn, CommandPtr command, EpollInstancePtr epoll_instance){
    if(!conn || conn->isClosed()){
        return "Error: Invalid connection";
    }
    
    if(!epoll_instance){
        return "Error: Server error - no epoll instance";
    }

    int fd = conn->getFd();
    auto& userMgr = UserManager::getInstance();
    
    if(!userMgr.isLoggedIn(fd)){
        return "Error: Please login first";
    }

    if(command->args.size() < 2){
        return "Error: Usage: /private_chat <username> <message>";
    }

    std::string target_username = command->args[0];
    auto target_fd_opt = userMgr.getFd(target_username);
    
    if(!target_fd_opt.has_value()){
        return "Error: User not found: " + target_username;
    }

    auto& room = PublicChatRoom::getInstance();
    if(room.isParticipant(target_fd_opt.value())){
        return "Error: This user is in a public chat room";
    }

    if(!epoll_instance->isEpollMember(target_fd_opt.value())){
        return "Error: This client does not exist";
    }

    std::string my_username = userMgr.getUsername(fd).value();
    std::string full_message = "";
    for(size_t i = 1; i < command->args.size(); i++){
        if(i > 1) full_message += " ";
        full_message += command->args[i];
    }
    
    full_message = MessageUtils::sanitize(full_message);

    std::string timestamp = TimeUtils::getCurrentTimestamp();
    
    return "[" + timestamp + "] [Private from " + my_username + "]: " + full_message;
}