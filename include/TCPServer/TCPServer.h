#pragma once
#include "Epoll.h"
#include "ThreadPool.h"
#include <arpa/inet.h>

class TCPServer{
    private:
        int listen_fd;
        EpollInstance& epoll_instance;
        ThreadPool& thread_pool_instance;

        void onAccept(int fd);
        void onRead(int clientFd);

    public:
        TCPServer(const sockaddr_in& addr, EpollInstance& epoll, ThreadPool& thread_pool);
        ~TCPServer();

        void startServer();
        void stopServer();
};