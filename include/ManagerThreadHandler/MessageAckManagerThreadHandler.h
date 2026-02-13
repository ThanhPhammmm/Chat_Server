#pragma once

#include <thread>
#include <atomic>
#include <memory>

class MessageAckThreadHandler{
    private:
        std::thread worker_thread;
        std::atomic<bool> running{false};
        
        void run();
        
    public:
        MessageAckThreadHandler();
        ~MessageAckThreadHandler();
        
        void start();
        void stop();
};

using MessageAckThreadHandlerPtr = std::shared_ptr<MessageAckThreadHandler>;