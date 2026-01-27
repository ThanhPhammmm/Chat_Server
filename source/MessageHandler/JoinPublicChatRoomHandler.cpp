#include "JoinPublicChatRoomHandler.h"
#include "PublicChatRoom.h"

std::string JoinPublicChatHandler::handleMessage(ConnectionPtr conn, CommandPtr command, EpollInstancePtr epoll_instance){
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

    return std::to_string(fd) + " joined Public Chat Room. Current Members: " + std::to_string(room.getParticipantsCount());
}