#include "Epoll.h"
#include "Logger.h"

EpollInstance::EpollInstance(){
    epfd = epoll_create1(0);
    if(epfd < 0){
        perror("epoll_create1");
        throw std::runtime_error("Failed to create epoll instance");
    }
}

EpollInstance::~EpollInstance(){
    stop();
    
    int wait_count = 0;
    const int MAX_WAIT = 50;
    while(!should_stop.load(std::memory_order_acquire) && wait_count++ < MAX_WAIT){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    if(wait_count >= MAX_WAIT){
        std::cerr << "[WARNING] EpollInstance destructor timeout waiting for run() to finish" << std::endl;
    }
    
    std::lock_guard<std::mutex> lock(handlers_mutex);
    
    for(auto& pair : connections){
        if(pair.second){
            pair.second->close();
        }
    }
    connections.clear();
    handlers.clear();
    
    if(epfd >= 0){
        close(epfd);
        epfd = -1;
    }
}

// EpollInstance& EpollInstance::GetInstance(){
//     static EpollInstance epfd;
//     return epfd;
// }

void EpollInstance::addFd(int fd, Callback cb, ConnectionPtr conn){
    if(should_stop.load(std::memory_order_acquire)){
        std::cerr << "[WARNING] Attempted to add fd to stopped EpollInstance" << std::endl;
        return;
    }

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
        epoll_fds.insert(fd);
    }
}

void EpollInstance::removeFd(int fd){
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
    
    ConnectionPtr conn;
    {
        std::lock_guard<std::mutex> lock(handlers_mutex);
        handlers.erase(fd);
        epoll_fds.erase(fd);
        
        auto it = connections.find(fd);
        if(it != connections.end()){
            conn = it->second;
            connections.erase(it);
        }
    }
    if(conn && !conn->isClosed()){
        conn->close();
    }
    LOG_INFO_STREAM("Client disconnected fd=" << fd);
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
    epoll_event events[MAX_EVENTS];

    while(!should_stop.load(std::memory_order_acquire)){
        int n = epoll_wait(epfd, events, 1024, 1000);
        
        if(n < 0){
            if(errno == EINTR) continue;
            LOG_ERROR_STREAM("epoll_wait failed: " << strerror(errno));
            break;
        }
        
        for(int i = 0; i < n; i++){
            if(should_stop.load(std::memory_order_acquire)) break;

            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;
            
            if(ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)){
                LOG_DEBUG_STREAM("Epoll error event on fd=" << fd);
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
            
            if(handler){
                try{
                    handler(fd);
                }
                catch(const std::exception& e){
                    LOG_ERROR_STREAM("Handler exception for fd=" << fd << ": " << e.what());
                    removeFd(fd);
                }
                catch(...){
                    LOG_ERROR_STREAM("Unknown handler exception for fd=" << fd);
                    removeFd(fd);
                }
            }
        }
    }
    LOG_INFO_STREAM("[EpollInstance] Run loop exited");
}

void EpollInstance::stop(){
    should_stop.store(true, std::memory_order_release);
}

bool EpollInstance::isStopped(){
    return should_stop.load(std::memory_order_acquire);
}

std::vector<ConnectionPtr> EpollInstance::getAllConnections(){
    std::lock_guard<std::mutex> lock(handlers_mutex);
    std::vector<ConnectionPtr> conns;
    conns.reserve(connections.size());

    for(const auto& pair : connections){
        if(pair.second && !pair.second->isClosed()){
            conns.push_back(pair.second);
        }
    }
    return conns;
}

bool EpollInstance::isEpollMember(int fd){
    std::lock_guard<std::mutex> lock(handlers_mutex);
    return epoll_fds.find(fd) != epoll_fds.end();
}