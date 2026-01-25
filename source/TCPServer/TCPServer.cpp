#include "TCPServer.h"

static void setNonBlock(int fd){
    int flags = fcntl(fd, F_GETFL, 0);
    if(flags < 0){
        perror("fcntl F_GETFL");
        return;
    }
    if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0){
        perror("fcntl F_SETFL");
    }
}

TCPServer::TCPServer(const sockaddr_in& addr, 
                     EpollPtr epoll, 
                     ThreadPoolPtr thread_pool,
                     std::shared_ptr<MessageQueue<Message>> to_router)
    : epoll_instance(epoll), 
      thread_pool_instance(thread_pool),
      to_router_queue(to_router){ 
    
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_fd < 0){
        perror("socket");
        throw std::runtime_error("Failed to create socket");
    }
    
    int opt = 1;
    if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        perror("setsockopt");
        close(listen_fd);
        throw std::runtime_error("Failed to set socket options");
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = addr.sin_addr.s_addr;
    server_addr.sin_port = addr.sin_port;

    if(bind(listen_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror("bind");
        close(listen_fd);
        throw std::runtime_error("Failed to bind socket");
    }
    
    if(listen(listen_fd, SOMAXCONN) < 0){
        perror("listen");
        close(listen_fd);
        throw std::runtime_error("Failed to listen on socket");
    }
    
    setNonBlock(listen_fd);
    
    LOG_INFO_STREAM("[INFO] Server listening on port " << ntohs(addr.sin_port));}

TCPServer::~TCPServer(){
    if(listen_fd >= 0){
        close(listen_fd);
    }
}

void TCPServer::onRead(int clientFd){
    ConnectionPtr conn = epoll_instance->getConnection(clientFd);
    if(!conn || conn->isClosed()){
        return;
    }
    
    while(true){
        char buf[4096];
        ssize_t n = recv(clientFd, buf, sizeof(buf), 0);
        
        if(n < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                break;
            }
            
            perror("recv");
            
            // Message msg;
            // msg.type = MessageType::CLIENT_DISCONNECTED;
            // msg.payload = ClientDisconnected{clientFd};
            // to_router_queue->push(std::move(msg));
            
            // epoll_instance->removeFd(clientFd);
            return;
        }
        
        if(n == 0){
            // std::cout << "[INFO] Client closed connection fd=" << clientFd << std::endl;
            
            // Message msg;
            // msg.type = MessageType::CLIENT_DISCONNECTED;
            // msg.payload = ClientDisconnected{clientFd};
            // to_router_queue->push(std::move(msg));
            
            // epoll_instance->removeFd(clientFd);
            return;
        }
        
        std::string content(buf, n);
        
        Message msg;
        msg.type = MessageType::INCOMING_MESSAGE;
        msg.payload = IncomingMessage{conn, content, clientFd};
        to_router_queue->push(std::move(msg));
        
        LOG_DEBUG_STREAM("[TCPServer] Pushed message from fd=" << clientFd
                        << " to router queue");
    }
}

void TCPServer::onAccept(int fd){
    while(true){
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        
        int cfd = accept(fd, (sockaddr*)&client_addr, &client_len);
        
        if(cfd < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                break;
            }
            perror("accept");
            break;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        LOG_INFO_STREAM("New connection from "
                        << client_ip << ":"
                        << ntohs(client_addr.sin_port)
                        << " fd=" << cfd);

        setNonBlock(cfd);

        auto conn = std::make_shared<Connection>(cfd);
        
        std::weak_ptr<TCPServer> weak_this = shared_from_this();
        
        epoll_instance->addFd(cfd, [weak_this](int clientFd){
            if(auto self = weak_this.lock()){
                self->onRead(clientFd);
            }
        }, conn);
    }
}

void TCPServer::startServer(){
    std::weak_ptr<TCPServer> weak_this = shared_from_this();
    
    epoll_instance->addFd(listen_fd, [weak_this](int fd){
        if(auto self = weak_this.lock()){
            self->onAccept(fd);
        }
    });
    
    LOG_INFO_STREAM("Server started successfully");
}

void TCPServer::stopServer(){
    LOG_INFO_STREAM("Stopping server...");
    epoll_instance->stop();
}