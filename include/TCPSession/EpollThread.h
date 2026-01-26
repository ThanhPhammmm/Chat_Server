#pragma once

#include "Epoll.h"
#include "MessageQueue.h"
#include "Message.h"
#include <thread>
#include <atomic>

class EpollThread{
    private:
        EpollInstancePtr epoll_instance;
        std::thread worker_thread;
        std::atomic<bool> running{false};   
    
        void run();
    public:
        EpollThread(EpollInstancePtr epoll);
        void start();
        void stop();
        EpollInstancePtr getEpoll();
};

using EpollThreadPtr = std::shared_ptr<EpollThread>;