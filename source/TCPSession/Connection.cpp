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

int Connection::getFd() const{
    return fd;
}

bool Connection::isClosed() const{
    return closed.load(std::memory_order_acquire);
}

void Connection::close(){
    bool expected = false;
    if(closed.compare_exchange_strong(expected, true, std::memory_order_acq_rel)){
        if(fd >= 0){
            std::cout << "[INFO] Closing connection fd=" << fd << std::endl;
            
            ::shutdown(fd, SHUT_RDWR);
            
            if(::close(fd) < 0){
                perror("close");
            }
            
            fd = -1;
        }
    }
}
