#include "LeavePublicChatRoomHandler.h"
#include "PublicChatRoom.h"

std::string LeavePublicChatHandler::handleMessage(ConnectionPtr conn, CommandPtr command, EpollInstancePtr epoll_instance){
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
        return "You are not in the public chat room.";
    }
    
    room.leave(fd);
    
    return "Left public chat room. Current Members: " + std::to_string(room.getParticipantsCount());
}