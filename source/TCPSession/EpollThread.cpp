#include "EpollThread.h"
#include "Epoll.h"

EpollThread::EpollThread(EpollPtr epoll) : epoll_instance(epoll) {}

void EpollThread::start() {
    running.store(true);
    worker_thread = std::thread([this]{ run(); });
}

void EpollThread::stop() {
        running.store(false);
        epoll_instance->stop();
        if(worker_thread.joinable()) {
            worker_thread.join();
        }
}

void EpollThread::run() {
    std::cout << "┌────────────────────────────────────┐\n";
    std::cout << "│ [EpollThread] Started              │\n";
    std::cout << "│ TID: " << std::this_thread::get_id() << "           │\n";
    std::cout << "└────────────────────────────────────┘\n";
    
    epoll_instance->run();
    
    std::cout << "[EpollThread] Stopped\n";
}

EpollPtr EpollThread::getEpoll() {
    return epoll_instance;
}


