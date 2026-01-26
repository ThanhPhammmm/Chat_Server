#include "LeavePublicChatRoomHandler.h"
#include "PublicChatRoom.h"

bool LeavePublicChatHandler::canHandle(CommandType type){
    return type == CommandType::LEAVE_PUBLIC_CHAT_ROOM;
}

std::string LeavePublicChatHandler::handleMessage(ConnectionPtr conn, CommandPtr command, EpollInstancePtr epoll_instance){
    int fd = conn->getFd();
    PublicChatRoom tmp;
    auto& room = tmp.getInstance();
    
    if(!room.isParticipant(fd)){
        return "You are not in the public chat room.";
    }
    
    room.leave(fd);
    
    return "Left public chat room.";
}