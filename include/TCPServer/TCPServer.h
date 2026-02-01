#pragma once

#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <iostream>
#include <cstring>
#include <cerrno>
#include <memory>
#include <chrono>
#include <thread>
#include <csignal>

#include "EpollThread.h"
#include "ChatControllerThread.h"
#include "PublicChatThreadHandler.h"
#include "PublicChatHandler.h"
#include "Responser.h"
#include "MessageQueue.h"
#include "Message.h"
#include "MessageThreadHandler.h"
#include "Logger.h"
#include "ListUsersHandler.h"
#include "ListUsersThreadHandler.h"
#include "ThreadPool.h"
#include "Connection.h"
#include "Epoll.h"
#include "Command.h"
#include "CommandParser.h"
#include "BaseThreadHandler.h"
#include "MessageHandler.h"
#include "JoinPublicChatRoomHandler.h"
#include "JoinPublicChatRoomThreadHandler.h"
#include "LeavePublicChatRoomHandler.h"
#include "LeavePublicChatRoomThreadHandler.h"
#include "PrivateChatHandler.h"
#include "PrivateChatThreadHandler.h"
#include "RegisterAccountThreadHandler.h"
#include "RegisterAccountHandler.h"
#include "DataBaseThread.h"
#include "LoginChatHandler.h"
#include "LoginChatThreadHandler.h"
#include "LogoutChatHandler.h"
#include "LogoutChatThreadHandler.h"

#define BUFFER_SIZE 4096
#define MAX_EVENTS 1024
constexpr size_t MAX_MESSAGE_SIZE = 1024 * 1024;  // 1MB limit

class TCPServer : public std::enable_shared_from_this<TCPServer>{
    private:
        int listen_fd;
        EpollInstancePtr epoll_instance;
        ThreadPoolPtr thread_pool_instance;
        std::shared_ptr<MessageQueue<Message>> to_router_queue;

        void onAccept(int fd);
        void onRead(int clientFd);

    public:
        TCPServer(const sockaddr_in& addr, 
                EpollInstancePtr epoll, 
                ThreadPoolPtr thread_pool,
                std::shared_ptr<MessageQueue<Message>> to_router);
        ~TCPServer();

        void startServer();
        void stopServer();
};

using TCPServerPtr = std::shared_ptr<TCPServer>;