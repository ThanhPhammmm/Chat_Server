#pragma once

#include "Epoll.h"
#include "MessageQueue.h"
#include "Message.h"
#include <thread>
#include <atomic>

class EpollThread{
    private:
        EpollPtr epoll_instance;
        std::thread worker_thread;
        std::atomic<bool> running{false};   
    
        void run();
    public:
        EpollThread(EpollPtr epoll);
        void start();
        void stop();
        EpollPtr getEpoll();
};

using EpollThreadPtr = std::shared_ptr<EpollThread>;