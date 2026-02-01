#include "ListUsersHandler.h"
#include "ListUsersThreadHandler.h"
#include "UserManager.h"

std::string ListUsersHandler::handleMessage(ConnectionPtr conn, CommandPtr command, EpollInstancePtr epoll_instance){
    (void)command;

    if(!conn || conn->isClosed()){
        return "Error: Invalid connection";
    }
    
    if(!epoll_instance){
        return "Error: Server error - no epoll instance";
    }

    auto& userMgr = UserManager::getInstance();
    auto all_users = userMgr.getAllLoggedInUsers();
    
    std::string user_list = "Online Users:\n";
    
    for(const auto& [fd, username] : all_users){
        user_list += "  - " + username + " (fd=" + std::to_string(fd) + ")\n";
    }
    
    if(all_users.empty()){
        user_list += "  (none)\n";
    }
    
    return user_list;
}