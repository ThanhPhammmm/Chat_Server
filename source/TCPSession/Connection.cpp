#include "Connection.h"

Connection::Connection(int socket_fd) 
    : fd(socket_fd), closed(false){
    if(fd < 0){
        std::cerr << "[WARNING] Connection created with invalid fd: " << fd << std::endl;
    }
}

Connection::~Connection(){
    close();
}

Connection::Connection(Connection&& other) noexcept 
    : fd(other.fd), closed(other.closed.load()){
    other.fd = -1;
    other.closed.store(true);
}

Connection& Connection::operator=(Connection&& other) noexcept{
    if(this != &other){
        close();
        
        fd = other.fd;
        closed.store(other.closed.load());
        
        other.fd = -1;
        other.closed.store(true);
    }
    return *this;
}

int Connection::getFd(){
    return fd;
}

bool Connection::isClosed(){
    return closed.load(std::memory_order_acquire);
}

void Connection::close(){
    bool expected = false;
    if(closed.compare_exchange_strong(expected, true, std::memory_order_acq_rel)){
        if(fd >= 0){
            int shutdown_result = ::shutdown(fd, SHUT_RDWR);
            if(shutdown_result < 0 && errno != ENOTCONN){
                std::cerr << "[WARNING] Shutdown error on fd=" << fd 
                         << ": " << strerror(errno) << std::endl;
            }
            
            int close_result = ::close(fd);
            if(close_result < 0){
                std::cerr << "[WARNING] Close error on fd=" << fd 
                         << ": " << strerror(errno) << std::endl;
            }
            fd = -1;
        }
    }
    // If compare_exchange failed, another thread already closed it
}
