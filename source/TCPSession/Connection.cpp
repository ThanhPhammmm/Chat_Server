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
    }
}