#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <thread>
#include <atomic>
#include <string>
#include <chrono>

std::atomic<bool> running{true};

void setNonBlocking(int fd){
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void setBlocking(int fd){
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
}

void receiveThread(int sock){
    char buffer[4096];
    
    while(running.load()){
        memset(buffer, 0, sizeof(buffer));
        int n = recv(sock, buffer, sizeof(buffer) - 1, 0);
        
        if(n < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            if(running.load()){
                perror("recv");
            }
            break;
        }
        
        if(n == 0){
            std::cout << "\n\n╔════════════════════════════════════╗\n";
            std::cout <<     "║  Server closed connection          ║\n";
            std::cout <<     "╚════════════════════════════════════╝\n";
            running.store(false);
            break;
        }
        
        buffer[n] = '\0';
        
        std::cout << "\r\033[K";  // Clear line
        std::cout << "📨 [Server]: " << buffer << std::endl;
        std::cout << "💬 You: " << std::flush;
    }
}

int main(){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0){
        perror("socket");
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    
    std::cout << "Enter server IP [type 127.0.0.1]: ";
    std::string server_ip;
    std::getline(std::cin, server_ip);
    if(server_ip.empty()) server_ip = "127.0.0.1";
    
    if(inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0){
        perror("inet_pton");
        close(sock);
        return 1;
    }

    std::cout << "Connecting to " << server_ip << ":8080...\n";
    if(connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror("connect");
        close(sock);
        return 1;
    }

    std::cout << "\n╔════════════════════════════════════╗\n";
    std::cout <<     "║  Connected to server successfully  ║\n";
    std::cout <<     "║  Type 'exit' to disconnect         ║\n";
    std::cout <<     "╚════════════════════════════════════╝\n\n";

    std::thread recv_thread(receiveThread, sock);

    std::string msg;
    while(running.load()){
        std::cout << "💬 You: " << std::flush;
        std::getline(std::cin, msg);
        
        if(!running.load()) break;
        
        if(msg == "exit" || msg == "quit"){
            running.store(false);
            break;
        }
        
        if(msg.empty()) continue;
        
        ssize_t sent = send(sock, msg.c_str(), msg.size(), 0);
        if(sent < 0){
            perror("send");
            running.store(false);
            break;
        }
    }

    running.store(false);
    shutdown(sock, SHUT_RDWR);
    recv_thread.join();
    close(sock);
    
    std::cout << "\n╔════════════════════════════════════╗\n";
    std::cout <<     "║  Disconnected from server          ║\n";
    std::cout <<     "╚════════════════════════════════════╝\n";
    
    return 0;
}