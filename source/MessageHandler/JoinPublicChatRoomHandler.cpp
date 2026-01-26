#include "JoinPublicChatRoomHandler.h"
#include "PublicChatRoom.h"

std::string JoinPublicChatHandler::handleMessage(ConnectionPtr conn, CommandPtr command, EpollInstancePtr epoll_instance){
    int fd = conn->getFd();

    PublicChatRoom tmp;
    auto& room = tmp.getInstance();

    if(room.isParticipant(fd)){
        return "You are already in the public chat room.";
    };

    room.join(fd);

    return "Joined Public Chat Room.Current Members: " + std::to_string(room.getParticipantsCount());
}

bool JoinPublicChatHandler::canHandle(CommandType type){
    return type == CommandType::JOIN_PUBLIC_CHAT_ROOM;
}