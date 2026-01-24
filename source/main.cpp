#include "TCPServer.h"
#include "Epoll.h"
#include "ThreadPool.h"
#include <csignal>
#include <iostream>

EpollPtr g_epoll = nullptr;

void signalHandler(int signum){
    std::cout << "\n[INFO] Received signal " << signum << ", shutting down...\n";
    if(g_epoll){
        g_epoll->stop();
    }
}

int main(){
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler); 
    signal(SIGPIPE, SIG_IGN);

    try{
        auto thread_pool = std::make_shared<ThreadPool>(4);
        auto epoll_instance = std::make_shared<EpollInstance>();
        g_epoll = epoll_instance;
        
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(8080);
        
        auto server = std::make_shared<TCPServer>(addr, epoll_instance, thread_pool);
        server->startServer();
        
        std::cout << "[INFO] Press Ctrl+C to stop\n";
        epoll_instance->run();
        
        std::cout << "[INFO] Server stopped\n";
        
    } 
    catch(const std::exception& e){
        std::cerr << "[ERROR] " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}