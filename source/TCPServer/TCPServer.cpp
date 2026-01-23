#include "TCPServer.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <iostream>

static void setNonBlock(int fd){
    fcntl(fd, F_SETFL, O_NONBLOCK);
}

TCPServer::TCPServer(const sockaddr_in& addr, EpollInstance& epoll, ThreadPool& thread_pool)
    : epoll_instance(epoll), 
      thread_pool_instance(thread_pool){ 
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = addr.sin_addr.s_addr;
    server_addr.sin_port = addr.sin_port;

    bind(listen_fd, (sockaddr*)&server_addr, sizeof(server_addr));
    listen(listen_fd, SOMAXCONN);
    setNonBlock(listen_fd);
}

TCPServer::~TCPServer(){
    close(listen_fd);
}

void TCPServer::onRead(int clientFd){
    char buf[4096];
    ssize_t n = recv(clientFd, buf, sizeof(buf), 0);
    if (n <= 0) return;

    std::string msg(buf, n);

    thread_pool_instance.submit([clientFd, msg](){
        std::cout << "[Client " << clientFd << "]: " << msg << std::endl;
        send(clientFd, msg.data(), msg.size(), 0);
    });
}

void TCPServer::onAccept(int fd){
    while(true){
        int cfd = accept(fd, nullptr, nullptr);
        if(cfd < 0) break;

        setNonBlock(cfd);

        epoll_instance.addFd(cfd, [this](int clientFd){
            onRead(clientFd);
        });
    }
}

void TCPServer::startServer(){
    epoll_instance.addFd(listen_fd, [this](int fd) {
        onAccept(fd);
    });
}