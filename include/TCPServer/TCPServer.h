#pragma once

#include "Epoll.h"
#include "ThreadPool.h"
#include "Connection.h"
#include "ChatControllerThread.h"
#include "MessageHandler.h"
#include "PublicChatHandlerThread.h"
#include "PublicChatHandler.h"
#include "MessageQueue.h"
#include "Message.h"
#include "Responser.h"
#include "EpollThread.h"

class TCPServer : public std::enable_shared_from_this<TCPServer>{
    private:
        int listen_fd;
        EpollPtr epoll_instance;
        ThreadPoolPtr thread_pool_instance;
        
        // Queue to send messages to Router
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