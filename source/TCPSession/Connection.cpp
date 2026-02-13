#include "Connection.h"

Connection::Connection(int socket_fd) : fd(socket_fd), closed(false){
    if(socket_fd < 0){
        std::cerr << "[WARNING] Connection created with invalid fd: " << socket_fd << std::endl;
    }
}

Connection::~Connection(){
    close();
}

Connection::Connection(Connection&& other) noexcept : fd(other.fd.load()), closed(other.closed.load()){
    other.fd.store(-1);
    other.closed.store(true);
}

Connection& Connection::operator=(Connection&& other) noexcept{
    if(this != &other){
        close();
        
        fd.store(other.fd.load());
        closed.store(other.closed.load());
        
        other.fd.store(-1);
        other.closed.store(true);
    }
    return *this;
}

int Connection::getFd(){
    return fd.load(std::memory_order_acquire);
}

bool Connection::isClosed(){
    return closed.load(std::memory_order_acquire);
}

void Connection::close(){
    std::lock_guard<std::mutex> lock(close_mutex);
    
    bool expected = false;
    if(closed.compare_exchange_strong(expected, true, std::memory_order_acq_rel)){
        int current_fd = fd.exchange(-1, std::memory_order_acq_rel);
        
        if(current_fd >= 0){
            int shutdown_result = ::shutdown(current_fd, SHUT_RDWR);
            if(shutdown_result < 0 && errno != ENOTCONN && errno != EBADF){
                std::cerr << "[WARNING] Shutdown error on fd=" << current_fd << ": " << strerror(errno) << std::endl;
            }
            
            int close_result = ::close(current_fd);
            if(close_result < 0){
                std::cerr << "[WARNING] Close error on fd=" << current_fd << ": " << strerror(errno) << std::endl;
            }
        }
        clearWriteQueue();
    }
}

void Connection::queueWrite(std::string data){
    if(data.empty()) return;
    std::lock_guard<std::mutex> lock(write_mutex);
    write_queue.push_back(std::move(data));
}

bool Connection::hasWriteData(){
    std::lock_guard<std::mutex> lock(write_mutex);
    return !write_queue.empty();
}

std::string Connection::popWriteData(){
    std::lock_guard<std::mutex> lock(write_mutex);
    
    if(!partial_write.empty()){
        return std::move(partial_write);
    }
    
    if(write_queue.empty()){
        return "";
    }
    
    std::string data = std::move(write_queue.front());
    write_queue.pop_front();
    return data;
}

size_t Connection::getWriteQueueSize(){
    std::lock_guard<std::mutex> lock(write_mutex);
    return write_queue.size();
}

void Connection::clearWriteQueue(){
    std::lock_guard<std::mutex> lock(write_mutex);
    write_queue.clear();
    partial_write.clear();
    writing.store(false, std::memory_order_release);
}