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

TCPServer::TCPServer(const sockaddr_in& addr, EpollInstancePtr epoll, std::shared_ptr<MessageQueue<Message>> to_router)
    : epoll_instance(epoll), 
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
    
    LOG_DEBUG_STREAM("[INFO] Server listening on port " << ntohs(addr.sin_port));}

TCPServer::~TCPServer(){
    if(listen_fd >= 0){
        close(listen_fd);
    }
}

void TCPServer::onRead(int clientFd){
    ConnectionPtr conn = epoll_instance->getConnection(clientFd);
    if(!conn || conn->isClosed()){
        LOG_DEBUG_STREAM("Connection not found or closed for fd=" << clientFd);
        return;
    }
    
    conn->updateActivity();

    while(true){
        char buf[BUFFER_SIZE];
        ssize_t n = recv(clientFd, buf, BUFFER_SIZE - 1, 0);
        
        if(n < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                break;
            }
            
            LOG_WARNING_STREAM("Recv error on fd=" << clientFd << ": " << strerror(errno));
            epoll_instance->removeFd(clientFd);
            return;
        }
        
        if(n == 0){
            epoll_instance->removeFd(clientFd);
            LOG_INFO_STREAM("Client disconnected fd=" << clientFd);
            return;
        }
        
        buf[n] = '\0';
        std::string received_data(buf, n);
        
        conn->appendReadBuffer(received_data);
        
        if(conn->getReadBufferSize() > MAX_MESSAGE_SIZE){
            LOG_WARNING_STREAM("Read buffer too large for fd=" << clientFd << " (" << conn->getReadBufferSize() << " bytes), closing connection");
            epoll_instance->removeFd(clientFd);
            return;
        }

        LOG_DEBUG_STREAM("[TCPServer] Received " << n << " bytes from fd=" << clientFd << ", buffer size now: " << received_data.size());
    }
    
    while(true){
        std::string complete_msg = conn->extractCompleteMessage();
        
        if(complete_msg.empty()){
            break;
        }
        
        LOG_DEBUG_STREAM("[TCPServer] Extracted complete message from fd=" << clientFd << ": '" << complete_msg << "'");
        
        if(complete_msg.length() >= 4 && complete_msg.compare(0, 4, "ACK|") == 0){
            if(complete_msg.length() > 4){
                std::string msg_id = complete_msg.substr(4);
                
                bool valid = false;
                if(msg_id.length() == 14 && msg_id.compare(0, 4, "MSG_") == 0){
                    valid = true;
                    for(size_t i = 4; i < 14; ++i){
                        if(!std::isdigit(msg_id[i])){
                            valid = false;
                            break;
                        }
                    }
                }
                
                if(valid){
                    MessageAckManager::getInstance().acknowledgeMessage(msg_id);
                    LOG_DEBUG_STREAM("[ACK] Received ACK for " << msg_id);
                }
                else{
                    LOG_WARNING_STREAM("[ACK] Malformed ACK message ID from fd=" << clientFd << ": '" << msg_id << "'");
                }
            }
            else{
                LOG_WARNING_STREAM("[ACK] Empty ACK message received from fd=" << clientFd);
            }
            continue;
        }
        
        bool has_invalid_chars = false;
        for(char c : complete_msg){
            if(c == '\0' || (c < 32 && c != '\n' && c != '\r' && c != '\t')){
                has_invalid_chars = true;
                break;
            }
        }
        
        if(has_invalid_chars){
            LOG_WARNING_STREAM("Invalid characters in message from fd=" << clientFd << ", ignoring");
            continue;
        }
        
        if(conn->isRateLimited()){
            LOG_WARNING_STREAM("[RATE_LIMIT] Client fd=" << clientFd << " is sending too fast, dropping message");
            std::string warning = "0|Warning: Rate limit exceeded. Slow down your messages.\n";
            send(clientFd, warning.c_str(), warning.size(), MSG_NOSIGNAL);
            continue;
        }
        
        conn->recordMessage();
        
        Message msg;
        msg.type = MessageType::INCOMING_MESSAGE;
        msg.payload = IncomingMessage{conn, complete_msg, clientFd};
        to_router_queue->push(std::move(msg));
        
        LOG_DEBUG_STREAM("[TCPServer] Pushed complete message from fd=" << clientFd << " to router queue");
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
            LOG_ERROR_STREAM("Accept error: " << strerror(errno));
            break;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        LOG_INFO_STREAM("New connection from " << client_ip << ":" << ntohs(client_addr.sin_port) << " fd=" << cfd);

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
}

void TCPServer::stopServer(){
    if(listen_fd >= 0){
        epoll_instance->removeFd(listen_fd);
        close(listen_fd);
        listen_fd = -1;
    }
}