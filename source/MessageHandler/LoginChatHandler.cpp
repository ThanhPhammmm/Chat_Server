#include "LoginChatHandler.h"
#include "UserManager.h"
#include "TimeUtils.h"

LoginChatHandler::LoginChatHandler(DataBaseThreadPtr db_thread) : db_thread(db_thread) {}

std::string LoginChatHandler::handleMessage(ConnectionPtr conn, CommandPtr command, EpollInstancePtr epoll_instance){
    (void)epoll_instance;

    if(!conn || conn->isClosed()){
        return "Error: Invalid connection";
    }
    
    int fd = conn->getFd();
    auto& userMgr = UserManager::getInstance();
    
    if(userMgr.isLoggedIn(fd)){
        return "Error: Already logged in as " + userMgr.getUsername(fd).value();
    }
    
    if(command->args.size() < 2){
        return "Error: Usage: /login <username> <password>";
    }
    
    std::string username = command->args[0];
    std::string password = command->args[1];
    
    if(userMgr.isUsernameLoggedIn(username)){
        return "Error: User already logged in from another connection";
    }
    
    auto req = std::make_shared<DBRequest>();
    req->type = DBOperationType::VERIFY_LOGIN;
    req->username = username;
    req->password = password;
    req->fd = fd;
    req->callback = [&](bool isSuccess, const std::string& msg){
        std::lock_guard<std::mutex> lock(result_mutex);
        login_success = isSuccess;
        result = msg;
        done = true;
        result_cv.notify_one();
    };
    
    db_thread->submitRequest(req);
    
    std::unique_lock<std::mutex> lock(result_mutex);
    result_cv.wait_for(lock, std::chrono::seconds(5), [&]{ return done; });
    
    if(!done){
        return "Error: Database timeout";
    }
    
    if(login_success){
        userMgr.loginUser(fd, username);
        std::string timestamp = TimeUtils::getCurrentTimestamp();
        return "[" + timestamp + "] Success: Logged in as " + username;
    }
    
    return result;
}