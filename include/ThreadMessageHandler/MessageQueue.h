#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <memory>

template<typename T>
class MessageQueue {
    private:
        std::queue<T> queue;
        std::mutex mtx;
        std::condition_variable cv;
        bool stopped = false;

    public:
        void push(T item) {
            {
                std::lock_guard<std::mutex> lock(mtx);
                if(stopped) return;
                queue.push(std::move(item));
            }
            cv.notify_one();
        }

        std::optional<T> pop(int timeout_ms = -1) {
            std::unique_lock<std::mutex> lock(mtx);
            
            if(timeout_ms < 0) {
                cv.wait(lock, [this]{ return stopped || !queue.empty(); });
            } else {
                cv.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                    [this]{ return stopped || !queue.empty(); });
            }
            
            if(stopped && queue.empty()) {
                return std::nullopt;
            }
            
            if(queue.empty()) {
                return std::nullopt;
            }
            
            T item = std::move(queue.front());
            queue.pop();
            return item;
        }

        void stop() {
            {
                std::lock_guard<std::mutex> lock(mtx);
                stopped = true;
            }
            cv.notify_all();
        }

        size_t size() {
            std::lock_guard<std::mutex> lock(mtx);
            return queue.size();
        }
};