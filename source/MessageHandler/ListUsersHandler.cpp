#include "ListUsersHandler.h"   

bool ListUsersHandler::canHandle(CommandType type){
    return type == CommandType::LIST_USERS;
}

std::string ListUsersHandler::handleMessage(ConnectionPtr conn, CommandPtr command, EpollPtr epoll_instance){
    auto all_users = epoll_instance->getAllConnections();

    std::string begin = std::to_string(conn->getFd()) + " requested users list successfully.\n";
    std::string user_list = "Connected Users: ";
    for(const auto& user : all_users){
        if(user->isClosed()) continue;
        user_list += std::to_string(user->getFd()) + "\n ";
    }
    user_list.pop_back(); // Remove the last newline

    return begin + user_list;
}