#pragma once

#include "MessageQueue.h"
#include "ThreadMessageHandler.h"
#include "MessageHandler.h"
#include <thread>
#include <atomic>
#include <memory>

class BaseThreadHandler{
    protected:
        MessageHandlerPtr message_handler;
        std::shared_ptr<MessageQueue<HandlerRequestPtr>> request_queue;
        std::shared_ptr<MessageQueue<HandlerResponsePtr>> response_queue;
        std::thread worker_thread;
        std::atomic<bool> running{false};
        std::string handler_name;
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