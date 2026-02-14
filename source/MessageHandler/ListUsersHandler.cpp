#include "ListUsersHandler.h"
#include "ListUsersThreadHandler.h"
#include "UserManager.h"
#include "TimeUtils.h"

std::string ListUsersHandler::handleMessage(ConnectionPtr conn, CommandPtr command, EpollInstancePtr epoll_instance){
    (void)command;

    if(!conn || conn->isClosed()){
        return "Error: Invalid connection";
    }
    
    if(!epoll_instance){
        return "Error: Server error - no epoll instance";
    }
    
    auto& userMgr = UserManager::getInstance();
    int fd = conn->getFd();
    if(!userMgr.isLoggedIn(fd)){
        return "Error: Please log in first" ;
    }

    auto all_users = userMgr.getAllLoggedInUsers();
    std::string timestamp = TimeUtils::getCurrentTimestamp();
    
    std::ostringstream oss;
    oss << "[" << timestamp << "] Online Users (" << all_users.size() << "): ";
    
    if(all_users.empty()){
        oss << "(none)";
    }
    else{
        bool first = true;
        for(const auto& [user_fd, username] : all_users){
            if(!first) oss << ", ";
            oss << username;
            first = false;
        }
    }
    
    return oss.str();
}