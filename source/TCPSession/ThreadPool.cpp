#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t n){
    try{
        workers.reserve(n);
        
        for(size_t i = 0; i < n; i++){
            workers.emplace_back([this]{
                while(true){
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(mtx);
                        cv.wait(lock, [this]{ return stop.load() || !tasks.empty(); });
                        
                        if(stop.load() && tasks.empty()) return;
                        
                        if(tasks.empty()) continue;
                        
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    
                    if(task){
                        try{
                            task();
                        } 
                        catch(const std::exception& e){
                            std::cerr << "[ERROR] Task exception: " << e.what() << std::endl;
                        }
                        catch(...){
                            std::cerr << "[ERROR] Unknown task exception" << std::endl;
                        }
                    }
                }
            });
        }
    }
    catch(...){
        stop.store(true);
        cv.notify_all();
        
        for(auto& t : workers){
            if(t.joinable()){
                t.join();
            }
        }
        
        throw;
    }
}

void ThreadPool::submit(std::function<void()> task){
    {
        std::lock_guard<std::mutex> lock(mtx);
        if(stop.load()){
            std::cerr << "[WARNING] Attempted to submit task to stopped ThreadPool" << std::endl;
            return;
        }
        tasks.push(std::move(task));
    }
    cv.notify_one();
}

ThreadPool::~ThreadPool(){
    stop.store(true, std::memory_order_release);
    cv.notify_all();
    
    for(auto& t : workers){
        if(t.joinable()){
            t.join();
        }
    }
}