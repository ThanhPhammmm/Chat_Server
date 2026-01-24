#pragma once

#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <iostream>
#include <cstring>
#include <cerrno>
#include <memory>

#include "Epoll.h"
#include "ThreadPool.h"
#include "Connection.h"
#include "MessageQueue.h"
#include "Message.h"

class TCPServer : public std::enable_shared_from_this<TCPServer>{
    private:
        int listen_fd;
        EpollPtr epoll_instance;
        ThreadPoolPtr thread_pool_instance;
        std::shared_ptr<MessageQueue<Message>> to_router_queue;

        void onAccept(int fd);
        void onRead(int clientFd);

    public:
        TCPServer(const sockaddr_in& addr, 
                EpollPtr epoll, 
                ThreadPoolPtr thread_pool,
                std::shared_ptr<MessageQueue<Message>> to_router);
        ~TCPServer();

        void startServer();
        void stopServer();
};

using TCPServerPtr = std::shared_ptr<TCPServer>;