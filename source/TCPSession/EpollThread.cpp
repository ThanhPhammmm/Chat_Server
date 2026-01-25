#include "EpollThread.h"
#include "Epoll.h"
#include "TCPServer.h"

EpollThread::EpollThread(EpollPtr epoll) : epoll_instance(epoll) {}

void EpollThread::start(){
    running.store(true);
    worker_thread = std::thread([this]{ run(); });
}

void EpollThread::stop(){
        running.store(false);
        epoll_instance->stop();
        if(worker_thread.joinable()){
            worker_thread.join();
        }
}

void EpollThread::run(){
    LOG_INFO_STREAM("┌────────────────────────────────────┐");
    LOG_INFO_STREAM("│       [EpollThread] Started        │");
    LOG_INFO_STREAM("│ TID: " << std::this_thread::get_id());
    LOG_INFO_STREAM("└────────────────────────────────────┘");
    
    epoll_instance->run();
    
    LOG_INFO_STREAM("[EpollThread] Stopped");
}

EpollPtr EpollThread::getEpoll(){
    return epoll_instance;
}


