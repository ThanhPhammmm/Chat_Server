#pragma once
#include <unordered_map>
#include <functional>

class EpollInstance{
    private:
        using Callback = std::function<void(int)>;

        int epfd;
        std::unordered_map<int, Callback> handlers;

    public:
        EpollInstance();
        ~EpollInstance();

        void addFd(int fd, Callback cb);
        void run();

};