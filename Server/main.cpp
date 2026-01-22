#include <iostream>
#include <thread>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>

using namespace std;

int main(){
    // Create Socket
    int ChatServer_FD = socket(AF_INET, SOCK_STREAM, 0);
    if(ChatServer_FD < 0){
        printf("FAILED1\n");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    int opt = 1;
    if(setsockopt(ChatServer_FD, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0){
        printf("FAILED2\n");
        close(ChatServer_FD);
        exit(EXIT_FAILURE);
    }

    // Bind socket
    int port = 8080;
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    if(bind(ChatServer_FD, (struct sockaddr*)&server, sizeof(server)) < 0){
        printf("FAILED3\n");
        close(ChatServer_FD);
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if(listen(ChatServer_FD, 16) < 0){
        printf("FAILED4\n");
        close(ChatServer_FD);
        exit(EXIT_FAILURE);
    }
}   

