#include "Epoll.h"
#include "Logger.h"
#include "UserManager.h"
#include "PublicChatRoom.h"

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
    read_handlers.clear();
    write_handlers.clear();
    
    if(epfd >= 0){
        close(epfd);
        epfd = -1;
    }
}

void EpollInstance::addFd(int fd, Callback read_cb, ConnectionPtr conn){
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
    read_handlers[fd] = read_cb;

    if(conn){
        connections[fd] = conn;
        epoll_fds.insert(fd);
    }
}

void EpollInstance::enableWrite(int fd, Callback write_cb){
    std::lock_guard<std::mutex> lock(handlers_mutex);
    
    if(read_handlers.find(fd) == read_handlers.end()){
        LOG_WARNING_STREAM("Attempted to enable write on unknown fd=" << fd);
        return;
    }
    
    if(write_handlers.find(fd) != write_handlers.end()){
        write_handlers[fd] = write_cb;
        return;
    }

    write_handlers[fd] = write_cb;
    
    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
    ev.data.fd = fd;
    
    if(epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) < 0){
        perror("epoll_ctl MOD (enable write)");
        write_handlers.erase(fd);
    }
    
    LOG_DEBUG_STREAM("Enabled EPOLLOUT for fd=" << fd);
}

void EpollInstance::disableWrite(int fd){
    std::lock_guard<std::mutex> lock(handlers_mutex);
    
    auto it = write_handlers.find(fd);
    if(it == write_handlers.end()){
        return;
    }

    write_handlers.erase(fd);
    
    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    ev.data.fd = fd;
    
    if(epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) < 0){
        perror("epoll_ctl MOD (disable write)");
        if(errno != EBADF && errno != ENOENT){
            LOG_ERROR_STREAM("epoll_ctl MOD (disable write) failed for fd=" << fd << ": " << strerror(errno));
        }
        return;
    }
    
    LOG_DEBUG_STREAM("Disabled EPOLLOUT for fd=" << fd);
}

bool EpollInstance::isWriteEnabled(int fd){
    std::lock_guard<std::mutex> lock(handlers_mutex);
    return write_handlers.find(fd) != write_handlers.end();
}

void EpollInstance::removeFd(int fd){
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
    
    ConnectionPtr conn;
    {
        std::lock_guard<std::mutex> lock(handlers_mutex);
        read_handlers.erase(fd);
        write_handlers.erase(fd);      
        epoll_fds.erase(fd);
        
        auto it = connections.find(fd);
        if(it != connections.end()){
            conn = it->second;
            connections.erase(it);
        }
    }

    UserManager::getInstance().logoutUser(fd);
    PublicChatRoom::getInstance().leave(fd);

    if(conn && !conn->isClosed()){
        conn->close();
    }
    LOG_DEBUG_STREAM("Clent disconnected fd =" << fd);
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
        int n = epoll_wait(epfd, events, MAX_EVENTS, 1000);
        
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
            
            if(ev & EPOLLIN){
                Callback read_handler;
                {
                    std::lock_guard<std::mutex> lock(handlers_mutex);
                    auto it = read_handlers.find(fd);
                    if(it == read_handlers.end()) continue;
                    read_handler = it->second;
                }
                
                if(read_handler){
                    try{
                        read_handler(fd);
                    }
                    catch(const std::exception& e){
                        LOG_ERROR_STREAM("Read handler exception for fd=" << fd << ": " << e.what());
                        removeFd(fd);
                    }
                }
            }
            
            if(ev & EPOLLOUT){
                Callback write_handler;
                {
                    std::lock_guard<std::mutex> lock(handlers_mutex);
                    auto it = write_handlers.find(fd);
                    if(it == write_handlers.end()) continue;
                    write_handler = it->second;
                }
                
                if(write_handler){
                    try{
                        write_handler(fd);
                    }
                    catch(const std::exception& e){
                        LOG_ERROR_STREAM("Write handler exception for fd=" << fd << ": " << e.what());
                        removeFd(fd);
                    }
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