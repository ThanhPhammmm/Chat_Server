#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <atomic>
#include <iostream>

class ThreadPool{
    private:
        std::vector<std::thread> workers;
        std::queue<std::function<void()>> tasks;
        std::mutex mtx;
        std::condition_variable cv;
        std::atomic<bool> stop{false};

    public:
        explicit ThreadPool(size_t n);
        ~ThreadPool();

        void submit(std::function<void()> task);
};

using ThreadPoolPtr = std::shared_ptr<ThreadPool>;
