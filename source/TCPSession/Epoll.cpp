#include "Epoll.h"

EpollInstance::EpollInstance(){
    epfd = epoll_create1(0);
    if(epfd < 0){
        perror("epoll_create1");
        throw std::runtime_error("Failed to create epoll instance");
    }
}

EpollInstance::~EpollInstance(){
    std::lock_guard<std::mutex> lock(handlers_mutex);
    connections.clear();
    handlers.clear();
    
    if(epfd >= 0){
        close(epfd);
    }
}

void EpollInstance::addFd(int fd, Callback cb, ConnectionPtr conn){
    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    ev.data.fd = fd;

    if(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) < 0){ 
        perror("epoll_ctl ADD");
        return;
    }
    
    std::lock_guard<std::mutex> lock(handlers_mutex);  
    handlers[fd] = cb;

    if(conn){
        connections[fd] = conn;
    }
}

void EpollInstance::removeFd(int fd){
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
    
    ConnectionPtr conn;
    {
        std::lock_guard<std::mutex> lock(handlers_mutex);
        handlers.erase(fd);
        
        auto it = connections.find(fd);
        if(it != connections.end()){
            conn = it->second;
            connections.erase(it);
        }
    }
    
    if(conn){
        conn->close();
    }
}

ConnectionPtr EpollInstance::getConnection(int fd){
    std::lock_guard<std::mutex> lock(handlers_mutex);
    auto it = connections.find(fd);
    if(it != connections.end()){
        return it->second;
    }
    return nullptr;
}

void EpollInstance::run(){
    epoll_event events[1024];

    while(!should_stop){
        int n = epoll_wait(epfd, events, 1024, 1000);
        
        if(n < 0){
            if(errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }
        
        for(int i = 0; i < n; i++){
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;
            
            if(ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)){
                std::cout << "[INFO] Connection error/closed fd=" << fd << std::endl;
                removeFd(fd);
                continue;
            }
            
            Callback handler;
            {
                std::lock_guard<std::mutex> lock(handlers_mutex);
                auto it = handlers.find(fd);
                if(it == handlers.end()) continue;
                handler = it->second;
            }
            
            handler(fd);
        }
    }
}

void EpollInstance::stop(){
    should_stop = true;
}