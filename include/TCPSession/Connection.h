#pragma once

#include <memory>
#include <atomic>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>

class Connection{
    private:
        int fd;
        std::atomic<bool> closed{false};

    public:
        explicit Connection(int socket_fd);
        ~Connection();

        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;

        Connection(Connection&& other) noexcept;
        Connection& operator=(Connection&& other) noexcept;

        int getFd() const;
        bool isClosed() const;
        void close();
};

using ConnectionPtr = std::shared_ptr<Connection>;