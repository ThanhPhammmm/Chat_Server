#pragma once

#include "MessageQueue.h"
#include "MessageThreadHandler.h"
#include "ThreadPool.h"
#include "Epoll.h"

class Responser{
    private:
        std::shared_ptr<MessageQueue<HandlerResponsePtr>> response_queue;
        ThreadPoolPtr thread_pool;
        EpollInstancePtr epoll_instance;
        std::thread worker_thread;
        std::atomic<bool> running{false};

        void run();
        void sendBackToClient(HandlerResponsePtr resp);
        void sendToClient(HandlerResponsePtr resp);
        void broadcastToRoom(HandlerResponsePtr resp);

    public:
        Responser(std::shared_ptr<MessageQueue<HandlerResponsePtr>> resp_queue, 
                  ThreadPoolPtr pool, 
                  EpollInstancePtr epoll);
        void start();
        void stop();
};

using ResponserPtr = std::shared_ptr<Responser>;