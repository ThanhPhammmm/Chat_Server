#include "PrivateChatHandler.h"
#include "PrivateChatThreadHandler.h"
#include "PublicChatRoom.h"

std::string PrivateChatHandler::handleMessage(ConnectionPtr conn, CommandPtr command, EpollInstancePtr epoll_instance){
    if(!conn || conn->isClosed()){
        return "Error: Invalid connection";
    }
    
    if(!epoll_instance){
        return "Error: Server error - no epoll instance";
    }
    if((command->args).size() <= 1){
        return "Error: Command is fault, check the command list again";
    }

    int received_fd = std::stoi(command->args[0]);

    PublicChatRoom tmp;
    auto& room = tmp.getInstance();
    
    if(room.isParticipant(received_fd)){
        return "Error: This user is in a public chat room";
    }

    if(!epoll_instance->isEpollMember(received_fd)){
        return "Error: This client does not exist";
    }

    std::string full_message = "";
    for(size_t i = 1; i < command->args.size(); i++){
        full_message += " " + command->args[i];
    }
    full_message = "[Private] fd=" 
                            + std::to_string(conn->getFd()) 
                            + ": " 
                            + full_message;

    return full_message;
}