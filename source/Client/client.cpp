#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

int main(){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0){
        perror("socket");
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    
    if(inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0){
        perror("inet_pton");
        close(sock);
        return 1;
    }

    if(connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror("connect");
        close(sock);
        return 1;
    }

    std::cout << "Connected to server\n";

    char buffer[1024];
    while(true){
        std::string msg;
        std::cout << "You: ";
        std::getline(std::cin, msg);

        if(msg == "exit") break;

        // ✓ Kiểm tra send
        ssize_t sent = send(sock, msg.c_str(), msg.size(), 0);
        if(sent < 0){
            perror("send");
            break;
        }

        memset(buffer, 0, sizeof(buffer));
        int n = recv(sock, buffer, sizeof(buffer) - 1, 0);  // ✓ Để chỗ cho null terminator
        
        if(n < 0){
            perror("recv");
            break;
        }
        
        if(n == 0){
            std::cout << "Server closed connection\n";
            break;
        }

        buffer[n] = '\0';  // ✓ Null terminate
        std::cout << "Server: " << buffer << "\n";
    }

    close(sock);
    std::cout << "Disconnected\n";
    return 0;
}