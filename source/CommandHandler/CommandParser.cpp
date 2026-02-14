#include "CommandParser.h"
#include "PublicChatRoom.h"

static std::vector<std::string> parseArguments(const std::string& input){
    std::vector<std::string> args;
    std::string current;
    bool in_quotes = false;
    bool escape_next = false;
    
    for(size_t i = 0; i < input.length(); i++){
        char c = input[i];
        
        if(escape_next){
            current += c;
            escape_next = false;
            continue;
        }
        
        if(c == '\\'){
            escape_next = true;
            continue;
        }
        
        if(c == '"'){
            in_quotes = !in_quotes;
            continue;
        }
        
        if(!in_quotes && (c == ' ' || c == '\t')){
            if(!current.empty()){
                args.push_back(current);
                current.clear();
            }
            continue;
        }
        
        current += c;
    }
    
    if(!current.empty()){
        args.push_back(current);
    }
    
    return args;
}

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
        
        auto& room = PublicChatRoom::getInstance();
        if(!room.isParticipant(conn->getFd())){
            cmd->type = CommandType::UNKNOWN;
        }
        else{
            cmd->type = CommandType::PUBLIC_CHAT;
        }
        cmd->args.push_back(message);
        return cmd;
    }

    std::vector<std::string> tokens = parseArguments(message);
    
    if(tokens.empty()){
        cmd->type = CommandType::UNKNOWN;
        return cmd;
    }
    
    std::string command_name = tokens[0];
    
    for(size_t i = 1; i < tokens.size(); i++){
        cmd->args.push_back(tokens[i]);
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

        auto& room = PublicChatRoom::getInstance();
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