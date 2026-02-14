#include "LoginChatHandler.h"
#include "UserManager.h"
#include "TimeUtils.h"
#include "Logger.h"

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

    /* Wait for database thread response */
    std::string result;
    std::mutex result_mutex;
    std::condition_variable result_cv;
    bool done = false;
    bool login_success = false;
    int user_id = -1;

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
    
    {
        std::unique_lock<std::mutex> lock(result_mutex);
        result_cv.wait_for(lock, std::chrono::seconds(5), [&]{ return done; });
    }

    if(!done){
        return "Error: Database timeout";
    }
    
    if(login_success){
        auto get_user_req = std::make_shared<DBRequest>();
        get_user_req->type = DBOperationType::GET_USER;
        get_user_req->username = username;
        
        bool user_fetched = false;
        get_user_req->callback = [&result_mutex, &user_id, &user_fetched, &result_cv, username](bool success, const std::string& msg){
            (void)msg;
            std::lock_guard<std::mutex> lock1(result_mutex);
            if(success){
                user_id = std::stoi(msg);
            }
            user_fetched = true;
            result_cv.notify_one();
        };
        
        db_thread->submitRequest(get_user_req);
        {
            std::unique_lock<std::mutex> lock1(result_mutex);
            result_cv.wait_for(lock1, std::chrono::seconds(2), [&]{ return user_fetched; });
        }   
        userMgr.loginUser(fd, username, user_id);
        
        std::string timestamp = TimeUtils::getCurrentTimestamp();
        LOG_INFO_STREAM("User logged in: " << username << " (fd=" << fd << ", user_id=" << user_id << ")");
        return "[" + timestamp + "] Success: Logged in as " + username + " (user_id=" + std::to_string(user_id) + ")";
    }
    
    return result;
}