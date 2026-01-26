#include "CommandParser.h"
#include "PublicChatRoom.h"

CommandPtr CommandParser::parse(std::string& message, IncomingMessage& incomming, EpollInstancePtr epoll_instance){
    auto cmd = std::make_shared<Command>();
    cmd->raw_message = message;

    if(message.empty()){
        return cmd;
    }

    PublicChatRoom tmp_public_chat_room;
    if(message[0] != '/'){
        if(tmp_public_chat_room.getInstance().isParticipant(epoll_instance->getConnection(incomming.fd)->getFd()) == 0){
            cmd->type = CommandType::UNKNOWN;
        }
        else{
            cmd->type = CommandType::PUBLIC_CHAT;
        }
        cmd->args.push_back(message);
        return cmd;
    }

    std::istringstream iss(message);
    std::string command_name;
    iss >> command_name;

    std::string arg;
    while(iss >> arg){
        cmd->args.push_back(arg);
    }

    if(command_name == "/login"){
        cmd->type = CommandType::LOGIN;
    } 
    else if(command_name == "/logout"){
        cmd->type = CommandType::LOGOUT;
    } 
    else if(command_name == "/list_users"){
        cmd->type = CommandType::LIST_USERS;
    } 
    else if(command_name == "/private_chat"){
        cmd->type = CommandType::PRIVATE_CHAT;
    } 
    else if(command_name == "/join_public_chat_room"){
        cmd->type = CommandType::JOIN_PUBLIC_CHAT_ROOM;
    }
    else if(command_name == "/leave_public_chat_room"){
        cmd->type = CommandType::LEAVE_PUBLIC_CHAT_ROOM;
    }
    else{
        cmd->type = CommandType::UNKNOWN;
    }

    return cmd;
}