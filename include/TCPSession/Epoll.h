#pragma once

#include "Connection.h"
#include "unordered_set"
#define MAX_EVENTS 1024

class EpollInstance : public std::enable_shared_from_this<EpollInstance>{
    private:
        using Callback = std::function<void(int)>;

        int epfd;
        std::unordered_set<int> epoll_fds;
        std::unordered_map<int, Callback> handlers;
        std::unordered_map<int, ConnectionPtr> connections;
        std::mutex handlers_mutex;
        std::atomic<bool> should_stop{false};

    public:
        EpollInstance();
        ~EpollInstance();

        // EpollInstance& GetInstance();
        void addFd(int fd, Callback cb, ConnectionPtr conn = nullptr);
        void removeFd(int fd);
        ConnectionPtr getConnection(int fd);
        void run();
        void stop();
        bool isStopped();
        bool isEpollMember(int fd);
        std::vector<ConnectionPtr> getAllConnections();
};

using EpollInstancePtr = std::shared_ptr<EpollInstance>;