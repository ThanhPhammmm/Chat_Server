#include "LogoutChatHandler.h"
#include "UserManager.h"
#include "TimeUtils.h"

std::string LogoutChatHandler::handleMessage(ConnectionPtr conn, CommandPtr command, EpollInstancePtr epoll_instance){
    (void)epoll_instance;
    (void)command;

    if(!conn || conn->isClosed()){
        return "Error: Invalid connection";
    }
    
    int fd = conn->getFd();
    auto& userMgr = UserManager::getInstance();
    
    if(!userMgr.isLoggedIn(fd)){
        return "Error: Not logged in";
    }
    
    std::string username = userMgr.getUsername(fd).value();
    userMgr.logoutUser(fd);
    
    std::string timestamp = TimeUtils::getCurrentTimestamp();
    return "[" + timestamp + "] Success: Logged out " + username;
}