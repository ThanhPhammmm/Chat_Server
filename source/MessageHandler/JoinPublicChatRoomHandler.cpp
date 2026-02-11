#include "JoinPublicChatRoomHandler.h"
#include "PublicChatRoom.h"
#include "UserManager.h"
#include "TimeUtils.h"

std::string JoinPublicChatHandler::handleMessage(ConnectionPtr conn, CommandPtr command, EpollInstancePtr epoll_instance){
    (void)epoll_instance;
    (void)command;

    if(!conn || conn->isClosed()){
        return "Error: Invalid connection";
    }

    int fd = conn->getFd();
    if(fd < 0){
        return "Error: Invalid file descriptor";
    }

    PublicChatRoom tmp;
    auto& room = tmp.getInstance();
    if(room.isParticipant(fd)){
        return "You are already in the public chat room.";
    };

    room.join(fd);
        
    auto& userMgr = UserManager::getInstance();
    std::string username = userMgr.getUsername(fd).value();

    std::string timestamp = TimeUtils::getCurrentTimestamp();
    
    return "[" + timestamp + "] " + username + " joined Public Chat Room. Current Members: " + std::to_string(room.getParticipantsCount());
}