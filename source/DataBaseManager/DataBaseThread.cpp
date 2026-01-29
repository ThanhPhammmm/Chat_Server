#include "DataBaseThread.h"
#include "Logger.h"

DataBaseThread::DataBaseThread(){
    request_queue = std::make_shared<MessageQueue<DBRequestPtr>>();
    db_manager = std::make_shared<DataBaseManager>("../DataBase/chat_server.db");
}

DataBaseThread::~DataBaseThread(){
    stop();
}

void DataBaseThread::start(){
    if(!db_manager->initialize()){
        LOG_ERROR("Failed to initialize database");
        return;
    }
    
    running.store(true);
    worker_thread = std::thread([this](){ run(); });
    LOG_INFO("DataBaseThread started");
}

void DataBaseThread::stop(){
    running.store(false);
    if(request_queue){
        request_queue->stop();
    }
    if(worker_thread.joinable()){
        worker_thread.join();
    }
    LOG_INFO("DataBaseThread stopped");
}

void DataBaseThread::submitRequest(DBRequestPtr req){
    if(request_queue){
        request_queue->push(req);
    }
}

void DataBaseThread::run(){
    while(running.load()){
        auto req_opt = request_queue->pop(100);
        if(!req_opt.has_value()){
            if(!running.load()) break;
            continue;
        }
        
        auto req = req_opt.value();
        if(req){
            processRequest(req);
        }
    }
}

void DataBaseThread::processRequest(DBRequestPtr req){
    bool success = false;
    std::string message;
    
    switch(req->type){
        case DBOperationType::REGISTER_USER:
            if(db_manager->usernameExists(req->username)){
                success = false;
                message = "Error: Username already exists";
            }
            else{
                success = db_manager->registerUser(req->username, req->password);
                message = success ? "Success: User registered" : "Error: Registration failed";
            }
            break;
            
        case DBOperationType::VERIFY_LOGIN:
            success = db_manager->verifyLogin(req->username, req->password);
            if(success){
                db_manager->updateLastLogin(req->username);
                message = "Success: Login successful";
            }
            else{
                message = "Error: Invalid username or password";
            }
            break;
            
        case DBOperationType::UPDATE_LAST_LOGIN:
            success = db_manager->updateLastLogin(req->username);
            message = success ? "Success: Last login updated" : "Error: Update failed";
            break;
            
        case DBOperationType::GET_USER:
            {
                auto user = db_manager->getUser(req->username);
                success = user.has_value();
                message = success ? "User found" : "User not found";
            }
            break;
            
        case DBOperationType::CHECK_USERNAME_EXISTS:
            success = db_manager->usernameExists(req->username);
            message = success ? "Username exists" : "Username available";
            break;
    }
    
    if(req->callback){
        req->callback(success, message);
    }
}