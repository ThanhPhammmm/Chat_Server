#pragma once
#include <unordered_map>
#include <functional>
#include <sys/epoll.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <mutex>

class EpollInstance{
    private:
        using Callback = std::function<void(int)>;

        int epfd;
        std::unordered_map<int, Callback> handlers;
        std::mutex handlers_mutex;

    public:
        EpollInstance();
        ~EpollInstance();

        void addFd(int fd, Callback cb);
        void removeFd(int fd); 
        void run();
        void stop();
        
    private:
        bool should_stop = false;
};