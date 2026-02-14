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
#include <deque>

class Connection{
    private:
        std::atomic<int> fd;
        std::atomic<bool> closed{false};
        mutable std::mutex close_mutex;

        std::deque<std::string> write_queue;
        std::mutex write_mutex;
        std::atomic<bool> writing{false};
        std::string partial_write;

        std::string read_buffer;
        std::mutex read_mutex;

        std::deque<std::chrono::steady_clock::time_point> message_timestamps;
        std::mutex rate_limit_mutex;
        static constexpr size_t MAX_MESSAGES_PER_WINDOW = 20;
        static constexpr std::chrono::seconds RATE_LIMIT_WINDOW{10};

        std::chrono::steady_clock::time_point last_activity;
        std::mutex activity_mutex;

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

        // Write
        void queueWrite(std::string data);
        bool hasWriteData();
        std::string popWriteData();
        size_t getWriteQueueSize();
        void clearWriteQueue();

        bool isWriting() const { return writing.load(); }
        void setWriting(bool val) { writing.store(val); }

        void setPartialWrite(std::string data) { partial_write = std::move(data); }
        std::string getPartialWrite() { return std::move(partial_write); }
        bool hasPartialWrite() const { return !partial_write.empty(); }

        // Read
        void appendReadBuffer(const std::string& data);
        std::string extractCompleteMessage();
        void clearReadBuffer();
        size_t getReadBufferSize();
        
        bool isRateLimited();
        void recordMessage();
        void updateActivity();
};

using ConnectionPtr = std::shared_ptr<Connection>;