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
    for(auto& pair : handlers){
        close(pair.first);
    }
    close(epfd);
}

void EpollInstance::addFd(int fd, Callback cb){
    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    ev.data.fd = fd;

    if(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) < 0){ 
        perror("epoll_ctl ADD");
        return;
    }
    
    std::lock_guard<std::mutex> lock(handlers_mutex);  
    handlers[fd] = cb;
}

void EpollInstance::removeFd(int fd){
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
    {
        std::lock_guard<std::mutex> lock(handlers_mutex);
        handlers.erase(fd);
    }
    
    close(fd);
    std::cout << "[INFO] Closed connection fd=" << fd << std::endl;
}

void EpollInstance::run(){
    epoll_event events[1024];

    while(!should_stop){  // ✓ Kiểm tra flag để có thể dừng
        int n = epoll_wait(epfd, events, 1024, 1000);  // ✓ Timeout 1s thay vì -1
        
        if(n < 0){
            if(errno == EINTR) continue;  // ✓ Signal interrupt - tiếp tục
            perror("epoll_wait");
            break;
        }
        
        for(int i = 0; i < n; i++){
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;
            
            // ✓ Kiểm tra lỗi hoặc disconnect
            if(ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)){
                std::cout << "[INFO] Connection error/closed fd=" << fd << std::endl;
                removeFd(fd);
                continue;
            }
            
            // ✓ Lock khi đọc handler
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