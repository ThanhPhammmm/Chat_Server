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
#include <unordered_set>

constexpr size_t BUFFER_SIZE = 4096;

std::atomic<bool> running{true};
std::string message_buffer;
std::unordered_set<std::string> processed_messages;

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

bool isValidMessageId(const std::string& msg_id){
    if(msg_id.length() != 14) return false;
    if(msg_id.substr(0, 4) != "MSG_") return false;
    
    for(size_t i = 4; i < 14; i++){
        if(!std::isdigit(msg_id[i])) return false;
    }
    
    return true;
}

void processMessage(int sock, const std::string& msg_id, const std::string& content){
    if(processed_messages.count(msg_id)){
        std::cout << "[DEBUG] Duplicate message " << msg_id << ", sending ACK again\n";
        sendAck(sock, msg_id);
        return;
    }
    
    std::cout << "\r\033[K";
    std::cout << "📨 " << content << std::endl;
    std::cout << "💬 You: " << std::flush;
    
    processed_messages.insert(msg_id);
    
    if(processed_messages.size() > 1000){
        auto it = processed_messages.begin();
        std::advance(it, 500);
        processed_messages.erase(processed_messages.begin(), it);
    }
    
    sendAck(sock, msg_id);
}

void parseBuffer(int sock){
    size_t pos;
    while((pos = message_buffer.find('\n')) != std::string::npos){
        std::string line = message_buffer.substr(0, pos);
        message_buffer.erase(0, pos + 1);
        
        size_t delim = line.find('|');
        if(delim == std::string::npos){
            std::cout << "[DEBUG] Invalid message format (no delimiter): " << line << "\n";
            continue;
        }
        
        std::string msg_id = line.substr(0, delim);
        std::string content = line.substr(delim + 1);
        
        if(!isValidMessageId(msg_id)){
            std::cout << "[DEBUG] Invalid MSG_ID: " << msg_id << "\n";
            continue;
        }
        
        processMessage(sock, msg_id, content);
    }
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
            
            message_buffer.clear();
            break;
        }
        
        message_buffer.append(buffer, n);
        
        parseBuffer(sock);
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