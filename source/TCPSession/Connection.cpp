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

// Write methods
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

// Read methods
void Connection::appendReadBuffer(const std::string& data){
    std::lock_guard<std::mutex> lock(read_mutex);
    read_buffer.append(data);
}

std::string Connection::extractCompleteMessage(){
    std::lock_guard<std::mutex> lock(read_mutex);
    
    size_t pos = read_buffer.find('\n');
    if(pos == std::string::npos){
        return "";
    }
    
    std::string message = read_buffer.substr(0, pos);
    
    read_buffer.erase(0, pos + 1);
    
    return message;
}

void Connection::clearReadBuffer(){
    std::lock_guard<std::mutex> lock(read_mutex);
    read_buffer.clear();
}

size_t Connection::getReadBufferSize(){
    std::lock_guard<std::mutex> lock(read_mutex);
    return read_buffer.size();
}

bool Connection::isRateLimited(){
    std::lock_guard<std::mutex> lock(rate_limit_mutex);
    
    auto now = std::chrono::steady_clock::now();
    auto cutoff = now - RATE_LIMIT_WINDOW;
    
    while(!message_timestamps.empty() && message_timestamps.front() < cutoff){
        message_timestamps.pop_front();
    }
    
    return message_timestamps.size() >= MAX_MESSAGES_PER_WINDOW;
}

void Connection::recordMessage(){
    std::lock_guard<std::mutex> lock(rate_limit_mutex);
    message_timestamps.push_back(std::chrono::steady_clock::now());
}

void Connection::updateActivity(){
    std::lock_guard<std::mutex> lock(activity_mutex);
    last_activity = std::chrono::steady_clock::now();
}