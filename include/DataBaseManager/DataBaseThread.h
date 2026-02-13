#pragma once

#include "DataBaseManager.h"
#include "MessageQueue.h"
#include <thread>
#include <atomic>
#include <functional>
#include <memory>

enum class DBOperationType{
    REGISTER_USER,
    VERIFY_LOGIN
};

struct DBRequest{
    DBOperationType type;
    std::string username;
    std::string password;
    int fd;
    int request_id;
    std::function<void(bool success, std::string&)> callback;
};

using DBRequestPtr = std::shared_ptr<DBRequest>;

class DataBaseThread{
    private:
        std::shared_ptr<MessageQueue<DBRequestPtr>> request_queue;
        std::thread worker_thread;
        std::atomic<bool> running{false};
        DatabaseManagerPtr db_manager;
        
        void run();
        void processRequest(DBRequestPtr req);
        
    public:
        DataBaseThread();
        ~DataBaseThread();
        
        void start();
        void stop();
        void submitRequest(DBRequestPtr req);
};

using DataBaseThreadPtr = std::shared_ptr<DataBaseThread>;