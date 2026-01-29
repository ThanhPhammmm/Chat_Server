#include "CommandParser.h"
#include "PublicChatRoom.h"

CommandPtr CommandParser::parse(std::string& message, IncomingMessage& incomming, EpollInstancePtr epoll_instance){
    auto cmd = std::make_shared<Command>();
    cmd->raw_message = message;

    if(message.empty()){
        cmd->type = CommandType::UNKNOWN;
        return cmd;
    }

    if(message[0] != '/'){
        if(!epoll_instance){
            cmd->type = CommandType::UNKNOWN;
            return cmd;
        }
        
        auto conn = epoll_instance->getConnection(incomming.fd);
        if(!conn){
            cmd->type = CommandType::UNKNOWN;
            return cmd;
        }
        
        PublicChatRoom temp_public_chat_room;
        auto& room = temp_public_chat_room.getInstance();
        if(!room.isParticipant(conn->getFd())){
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

    if(command_name == "/register"){
        cmd->type = CommandType::REGISTER;
    }
    else if(command_name == "/login"){
        cmd->type = CommandType::LOGIN;
    }
    else if(command_name == "/logout"){
        cmd->type = CommandType::LOGOUT;
    } 
    else if(command_name == "/list_online_users"){
        auto conn = epoll_instance->getConnection(incomming.fd);

        PublicChatRoom temp_public_chat_room;
        auto& room = temp_public_chat_room.getInstance();
        if(!room.isParticipant(conn->getFd())){
            cmd->type = CommandType::LIST_ONLINE_USERS;
        }
        else{
            cmd->type = CommandType::LIST_USERS_IN_PUBLIC_CHAT_ROOM;
        }
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