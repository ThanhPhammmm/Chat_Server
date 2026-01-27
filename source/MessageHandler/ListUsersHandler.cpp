#include "ListUsersHandler.h"
#include "ListUsersThreadHandler.h"

std::string ListUsersHandler::handleMessage(ConnectionPtr conn, CommandPtr command, EpollInstancePtr epoll_instance){
    if(!conn || conn->isClosed()){
        return "Error: Invalid connection";
    }
    
    if(!epoll_instance){
        return "Error: Server error - no epoll instance";
    }

    auto all_users = epoll_instance->getAllConnections();

    std::string begin = std::to_string(conn->getFd()) + " requested users list successfully.\n";
    std::string user_list = "Connected Users: ";
    int count = 0;

    for(const auto& user : all_users){
        if(!user || user->isClosed()) continue;
        if(count > 0) user_list += ", ";
        user_list += std::to_string(user->getFd());
        count++;
    }

    if(count == 0){
        user_list += "None";
    }

    return begin + user_list;
}