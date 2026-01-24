#pragma once

#include "Epoll.h"
#include "ThreadPool.h"
#include "Connection.h"
#include "ChatController.h"
#include "MessageHandler.h"
#include <PublicChatHandler.h>

class TCPServer : public std::enable_shared_from_this<TCPServer>{
    private:
        int listen_fd;
        EpollPtr epoll_instance;
        ThreadPoolPtr thread_pool_instance;
        ChatControllerPtr chat_controller; 

        void onAccept(int fd);
        void onRead(int clientFd);
        void sendResponse(ConnectionPtr conn, const std::string& response);
        void broadcastMessage(const std::string& message, int exclude_fd);

    public:
        TCPServer(const sockaddr_in& addr, EpollPtr epoll, ThreadPoolPtr thread_pool, ChatControllerPtr chat_controller);
        ~TCPServer();

        void startServer();
        void stopServer();
};

using TCPServerPtr = std::shared_ptr<TCPServer>;