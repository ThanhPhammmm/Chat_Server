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
            
        case DBOperationType::GET_USER:
            {
                auto user = db_manager->getUser(req->username);
                success = user.has_value();
                message = success ? "User found" : "User not found";
            }
            break;

        case DBOperationType::ADD_PENDING_MESSAGE:
            success = db_manager->addPendingMessage(req->message_id, req->sender_id, req->receiver_id);
            message = success ? "Message added to pending queue" : "Failed to add message";
            break;

        case DBOperationType::UPDATE_MESSAGE_STATUS:
            success = db_manager->updateMessageStatus(req->message_id, req->status);
            message = success ? "Message status updated" : "Failed to update status";
            
            if(success && req->status == "sent"){
                db_manager->incrementRetryCount(req->message_id);
            }
            break;

        case DBOperationType::DELETE_PENDING_MESSAGE:
            success = db_manager->deletePendingMessage(req->message_id);
            message = success ? "Message removed from pending queue" : "Failed to remove message";
            break;

        case DBOperationType::GET_PENDING_MESSAGES:
            {
                auto pending_msgs = db_manager->getPendingMessages();
                success = true;
                message = "Loaded " + std::to_string(pending_msgs.size()) + " pending messages";
                
                if(!pending_msgs.empty()){
                    LOG_INFO_STREAM("Found " << pending_msgs.size() << " pending messages:");
                    for(const auto& msg : pending_msgs){
                        LOG_DEBUG_STREAM("  - " << msg.message_id << " from user_id=" << msg.sender_id << " to user_id=" << msg.receiver_id<< " status=" << msg.status<< " retries=" << msg.retry_count);
                    }
                }
            }
            break;
    }
    
    if(req->callback){
        LOG_DEBUG_STREAM("User logged in1");
        req->callback(success, message);
        LOG_DEBUG_STREAM("User logged in2");
    }
}