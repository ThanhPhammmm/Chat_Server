#include "LeavePublicChatRoomHandler.h"
#include "PublicChatRoom.h"
#include "UserManager.h"
#include "TimeUtils.h"

std::string LeavePublicChatHandler::handleMessage(ConnectionPtr conn, CommandPtr command, EpollInstancePtr epoll_instance){
    (void)epoll_instance;
    (void)command;

    if(!conn || conn->isClosed()){
        return "Error: Invalid connection";
    }
    
    int fd = conn->getFd();
    if(fd < 0){
        return "Error: Invalid file descriptor";
    }

    auto& room = PublicChatRoom::getInstance();
    if(!room.isParticipant(fd)){
        return "You are not in the public chat room.";
    }
    
    room.leave(fd);

    auto& userMgr = UserManager::getInstance();
    std::string username = userMgr.getUsername(fd).value();
    std::string timestamp = TimeUtils::getCurrentTimestamp();
    
    return "[" + timestamp + "] " + username + " left public chat room. Current Members: " + std::to_string(room.getParticipantsCount());
}