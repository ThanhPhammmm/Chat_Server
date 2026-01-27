#include "EpollThread.h"
#include "Epoll.h"
#include "Logger.h"

EpollThread::EpollThread(EpollInstancePtr epoll) : epoll_instance(epoll) {}

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
    epoll_instance->run();
    
    LOG_INFO_STREAM("[EpollThread] Stopped");
}

EpollInstancePtr EpollThread::getEpoll(){
    return epoll_instance;
}


