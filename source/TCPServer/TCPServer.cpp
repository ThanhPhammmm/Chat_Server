#include "TCPServer.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <iostream>
#include <cstring>
#include <cerrno>

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

TCPServer::TCPServer(const sockaddr_in& addr, EpollInstance& epoll, ThreadPool& thread_pool)
    : epoll_instance(epoll), 
      thread_pool_instance(thread_pool){ 
    
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_fd < 0) {
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
    
    std::cout<< "[INFO] Server listening on port "<< ntohs(addr.sin_port) << std::endl;
}

TCPServer::~TCPServer(){
    if(listen_fd >= 0){
        close(listen_fd);
    }
}

void TCPServer::onRead(int clientFd){
    while(true){
        char buf[4096];
        ssize_t n = recv(clientFd, buf, sizeof(buf), 0);
        
        if(n < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                break;
            }
            perror("recv");
            epoll_instance.removeFd(clientFd);
            return;
        }
        
        if(n == 0){
            std::cout << "[INFO] Client closed connection fd=" << clientFd << std::endl;
            epoll_instance.removeFd(clientFd);
            return;
        }
        
        std::string msg(buf, n);
        
        thread_pool_instance.submit([clientFd, msg](){
            std::cout << "[Client " << clientFd << "]: " << msg << std::endl;
            
            const char* data = msg.data();
            size_t remaining = msg.size();
            
            while(remaining > 0){
                ssize_t sent = send(clientFd, data, remaining, MSG_NOSIGNAL);
                
                if(sent < 0){
                    if(errno == EAGAIN || errno == EWOULDBLOCK){
                        usleep(1000);
                        continue;
                    }
                    perror("send");
                    break;
                }
                
                data += sent;
                remaining -= sent;
            }
        });
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
        std::cout << "[INFO] New connection from " << client_ip << ":" << ntohs(client_addr.sin_port) << " fd=" << cfd << std::endl;

        setNonBlock(cfd);

        epoll_instance.addFd(cfd, [this](int clientFd){
            onRead(clientFd);
        });
    }
}

void TCPServer::startServer(){
    epoll_instance.addFd(listen_fd, [this](int fd){
        onAccept(fd);
    });
    std::cout << "[INFO] Server started successfully" << std::endl;
}

void TCPServer::stopServer(){
    std::cout << "[INFO] Stopping server..." << std::endl;
    epoll_instance.stop();
}