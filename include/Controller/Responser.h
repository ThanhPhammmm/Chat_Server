#pragma once

#include "MessageQueue.h"
#include "ThreadMessageHandler.h"
#include "ThreadPool.h"
#include "Epoll.h"
#include <thread>
#include <atomic>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <cerrno>

class Responser{
    private:
        std::shared_ptr<MessageQueue<HandlerResponsePtr>> response_queue;
        ThreadPoolPtr thread_pool;
        EpollPtr epoll_instance;
        std::thread worker_thread;
        std::atomic<bool> running{false};

        void run();
        void sendToClient(HandlerResponsePtr resp);

    public:
        Responser(std::shared_ptr<MessageQueue<HandlerResponsePtr>> resp_queue, 
                  ThreadPoolPtr pool, 
                  EpollPtr epoll);
        void start();
        void stop();
};

using ResponserPtr = std::shared_ptr<Responser>;