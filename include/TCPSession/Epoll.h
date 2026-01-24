#pragma once

#include "Connection.h"

class EpollInstance : public std::enable_shared_from_this<EpollInstance>{
    private:
        using Callback = std::function<void(int)>;

        int epfd;
        std::unordered_map<int, Callback> handlers;
        std::unordered_map<int, ConnectionPtr> connections;
        std::mutex handlers_mutex;
        std::atomic<bool> should_stop{false};

    public:
        EpollInstance();
        ~EpollInstance();

        void addFd(int fd, Callback cb, ConnectionPtr conn = nullptr);
        void removeFd(int fd);
        ConnectionPtr getConnection(int fd);
        void run();
        void stop();
        
        bool isStopped() const {
            return should_stop.load();
        }
        
        std::vector<ConnectionPtr> getAllConnections() {
            std::lock_guard<std::mutex> lock(handlers_mutex);
            std::vector<ConnectionPtr> conns;
            for(const auto& pair : connections) {
                conns.push_back(pair.second);
            }
            return conns;
        }
};

using EpollPtr = std::shared_ptr<EpollInstance>;