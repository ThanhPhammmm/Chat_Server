#pragma once

#include <unordered_map>
#include <functional>
#include <sys/epoll.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <mutex>
#include "Connection.h"

class EpollInstance : public std::enable_shared_from_this<EpollInstance>{
    private:
        using Callback = std::function<void(int)>;

        int epfd;
        std::unordered_map<int, Callback> handlers;
        std::unordered_map<int, ConnectionPtr> connections;
        std::mutex handlers_mutex;
        bool should_stop = false;

    public:
        EpollInstance();
        ~EpollInstance();

        void addFd(int fd, Callback cb, ConnectionPtr conn = nullptr);
        void removeFd(int fd);
        ConnectionPtr getConnection(int fd);
        void run();
        void stop();
};

using EpollPtr = std::shared_ptr<EpollInstance>;
