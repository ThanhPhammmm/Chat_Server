#pragma once

#include <memory>
#include <atomic>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <unordered_map>
#include <functional>
#include <sys/epoll.h>
#include <cstring>
#include <mutex>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <iostream>
#include <cstring>
#include <cerrno>
#include <csignal>
#include <variant>
#include <thread>

class Connection{
    private:
        std::atomic<int> fd;
        std::atomic<bool> closed{false};
        mutable std::mutex close_mutex;

    public:
        explicit Connection(int socket_fd);
        ~Connection();

        Connection(Connection&) = delete;
        Connection& operator=(Connection&) = delete;

        Connection(Connection&& other) noexcept;
        Connection& operator=(Connection&& other) noexcept;

        int getFd();
        bool isClosed();
        void close();
};

using ConnectionPtr = std::shared_ptr<Connection>;