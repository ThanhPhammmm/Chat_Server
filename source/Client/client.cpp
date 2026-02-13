#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <thread>
#include <atomic>
#include <string>
#include <chrono>
#include <sstream>

constexpr size_t BUFFER_SIZE = 4096;

std::atomic<bool> running{true};

void setNonBlocking(int fd){
    int flags = fcntl(fd, F_GETFL, 0);
    if(flags >= 0){
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

void sendAck(int sock, const std::string& msg_id){
    std::string ack = "ACK|" + msg_id;
    send(sock, ack.c_str(), ack.size(), MSG_NOSIGNAL);
}

void receiveThread(int sock){
    char buffer[BUFFER_SIZE];
    
    while(running.load()){
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t n = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        
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
        std::string received(buffer);
        
        // Parse message format: MSG_ID|content
        size_t delimiter_pos = received.find('|');
        if(delimiter_pos != std::string::npos){
            std::string msg_id = received.substr(0, delimiter_pos);
            std::string content = received.substr(delimiter_pos + 1);
            
            // Send ACK back to server
            sendAck(sock, msg_id);
            
            // Display the message
            std::cout << "\r\033[K";  // Clear line
            std::cout << "📨 " << content << std::endl;
            std::cout << "💬 You: " << std::flush;
        }
        else{
            // Fallback for messages without MSG_ID (shouldn't happen)
            std::cout << "\r\033[K";
            std::cout << "📨 " << received << std::endl;
            std::cout << "💬 You: " << std::flush;
        }
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
    
    std::cout << "Enter server IP [default: 127.0.0.1]: ";
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

    setNonBlocking(sock);
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
        
        if(msg.size() > 4000){
            std::cout << "⚠️  Message too long (max 4000 chars)\n";
            continue;
        }
        
        size_t total_sent = 0;
        size_t remaining = msg.size();
        const char* data = msg.c_str();
        
        while(remaining > 0 && running.load()){
            ssize_t sent = send(sock, data + total_sent, remaining, MSG_NOSIGNAL);
            
            if(sent < 0){
                if(errno == EAGAIN || errno == EWOULDBLOCK){
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                perror("send");
                running.store(false);
                break;
            }
            
            total_sent += sent;
            remaining -= sent;
        }
    }

    running.store(false);
    shutdown(sock, SHUT_RDWR);

    if(recv_thread.joinable()){
        recv_thread.join();
    }
    close(sock);
    
    std::cout << "\n╔════════════════════════════════════╗\n";
    std::cout <<     "║  Disconnected from server          ║\n";
    std::cout <<     "╚════════════════════════════════════╝\n";
    
    return 0;
}