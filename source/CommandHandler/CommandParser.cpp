#include "CommandParser.h"

CommandPtr CommandParser::parse(std::string& message){
    auto cmd = std::make_shared<Command>();
    cmd->raw_message = message;

    if(message.empty()){
        return cmd;
    }

    if(message[0] != '/'){
        // Default command
        cmd->type = CommandType::PUBLIC_CHAT;
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
    else if(command_name == "/public_chat"){
        cmd->type = CommandType::PUBLIC_CHAT;
    }
    else{
        cmd->type = CommandType::UNKNOWN;
    }

    return cmd;
}