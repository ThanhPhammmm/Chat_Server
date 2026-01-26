#pragma once

#include "MessageHandler.h"
#include "MessageThreadHandler.h"
#include "MessageQueue.h"
#include <thread>
#include <atomic>
#include <string>

class BaseThreadHandler{
    protected:
        MessageHandlerPtr message_handler;
        std::shared_ptr<MessageQueue<HandlerRequestPtr>> request_queue;
        std::shared_ptr<MessageQueue<HandlerResponsePtr>> response_queue;
        std::thread worker_thread;
        std::atomic<bool> running{false};
        std::string handler_name;
        
    protected:
        virtual void run() = 0;

    public:
        BaseThreadHandler(MessageHandlerPtr message_handler,
                          std::shared_ptr<MessageQueue<HandlerRequestPtr>> request_queue,
                          std::shared_ptr<MessageQueue<HandlerResponsePtr>> response_queue,
                          const std::string& handler_name);
        virtual ~BaseThreadHandler();
        
        void start();
        void stop();
};

using BaseThreadHandlerPtr = std::shared_ptr<BaseThreadHandler>;