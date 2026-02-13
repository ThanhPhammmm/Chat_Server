#include "RegisterAccountHandler.h"
#include <regex>
#include "TimeUtils.h"

RegisterAccountHandler::RegisterAccountHandler(DataBaseThreadPtr db_thread) : db_thread(db_thread) {}

std::string RegisterAccountHandler::handleMessage(ConnectionPtr conn, CommandPtr command, EpollInstancePtr epoll_instance){
    (void)epoll_instance;

    if(!conn || conn->isClosed()){
        return "Error: Invalid connection";
    }
    
    if(command->args.size() < 2){
        return "Error: Usage: /register <username> <password>";
    }
    
    std::string username = command->args[0];
    std::string password = command->args[1];
    
    if(username.length() < 3 || username.length() > 20){
        return "Error: Username must be 3-20 characters";
    }
    
    if(password.length() < 6){
        return "Error: Password must be at least 6 characters";
    }
    
    std::regex username_regex("^[a-zA-Z0-9_]+$");
    if(!std::regex_match(username, username_regex)){
        return "Error: Username can only contain letters, numbers, and underscores";
    }
    

    /* Wait for database thread response */
    std::string result;
    std::mutex result_mutex;
    std::condition_variable result_cv;
    bool done = false;
    bool register_success = false;

    auto req = std::make_shared<DBRequest>();
    req->type = DBOperationType::REGISTER_USER;
    req->username = username;
    req->password = password;
    req->fd = conn->getFd();
    req->callback = [&](bool isSuccess, std::string& msg){
        std::lock_guard<std::mutex> lock(result_mutex);
        register_success = isSuccess;
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

    if(register_success){
        std::string timestamp = TimeUtils::getCurrentTimestamp();
        return "[" + timestamp + "] " + result;
    }
    
    return result;
}