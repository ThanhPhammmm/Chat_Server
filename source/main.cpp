#include "TCPServer.h"
#include "Epoll.h"
#include "ThreadPool.h"

int main(){
    ThreadPool thread_pool(4);
    EpollInstance epoll_instance;
    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);
    TCPServer server(addr, epoll_instance, thread_pool);

    server.startServer();
    epoll_instance.run();
    return 0;
    
}
