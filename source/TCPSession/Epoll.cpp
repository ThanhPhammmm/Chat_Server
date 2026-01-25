#include "TCPServer.h"

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
    while(!should_stop.load() && wait_count++ < 10){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
    Callback handler;
    {
        std::lock_guard<std::mutex> lock(handlers_mutex);
        handlers.erase(fd);
        
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

    while(!should_stop.load()){
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
                    std::cerr << "[ERROR] Handler exception for fd=" << fd 
                             << ": " << e.what() << std::endl;
                    removeFd(fd);
                }
            }
        }
    }
}

void EpollInstance::stop(){
    should_stop.store(true);
}

bool EpollInstance::isStopped() const{
    return should_stop.load();
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