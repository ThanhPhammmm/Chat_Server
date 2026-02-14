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
                if(user.has_value()){
                    success = true;
                    message = std::to_string(user->id);
                }
                else{
                    success = false;
                    message = "User not found";
                }
            }
            break;

        case DBOperationType::ADD_PENDING_MESSAGE:
            success = db_manager->addPendingMessage(req->message_id, req->sender_id, req->receiver_id, req->message_content);
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

        case DBOperationType::GET_PENDING_MESSAGES_FOR_USER:
            {
                req->pending_messages = db_manager->getPendingMessagesForUser(req->user_id);
                success = true;
                message = "Loaded " + std::to_string(req->pending_messages.size()) + " pending messages for user";
                
                if(!req->pending_messages.empty()){
                    LOG_INFO_STREAM("Found " << req->pending_messages.size() << " pending messages for user_id=" << req->user_id);
                }
            }
            break;
    }
    
    if(req->callback){
        req->callback(success, message);
    }
}