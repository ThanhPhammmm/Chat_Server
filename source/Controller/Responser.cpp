#include "Responser.h"

Responser::Responser(std::shared_ptr<MessageQueue<HandlerResponsePtr>> resp_queue,
                     ThreadPoolPtr pool,
                     EpollPtr epoll)
    : response_queue(resp_queue),
      thread_pool(pool),
      epoll_instance(epoll) {}

void Responser::start(){
    running.store(true);
    worker_thread = std::thread([this]() { run(); });
}

void Responser::stop(){
    running.store(false);
    response_queue->stop();
    if(worker_thread.joinable()){
        worker_thread.join();
    }
}

void Responser::run(){
    std::cout << "┌────────────────────────────────────┐\n";
    std::cout << "│ [ResponseDispatcher] Started       │\n";
    std::cout << "│ TID: " << std::this_thread::get_id() << "           │\n";
    std::cout << "└────────────────────────────────────┘\n";

    while(running.load()){
        auto resp_opt = response_queue->pop(100);
        
        if(!resp_opt.has_value()) continue;
        
        auto resp = resp_opt.value();
        
        if(resp->is_broadcast){
            //broadcastMessage(resp);
        } 
        else {
            sendToClient(resp);
        }
    }

    std::cout << "[ResponseDispatcher] Stopped\n";
}

void Responser::sendToClient(HandlerResponsePtr resp){
    thread_pool->submit([conn = resp->connection, 
                        content = resp->response_message](){
        if(conn->isClosed()) return;
        
        int fd = conn->getFd();
        const char* data = content.data();
        size_t remaining = content.size();
        
        while(remaining > 0){
            if(conn->isClosed()) break;
            
            ssize_t sent = send(fd, data, remaining, MSG_NOSIGNAL);
            
            if(sent < 0){
                if(errno == EAGAIN || errno == EWOULDBLOCK) {
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

// void broadcastMessage(HandlerResponsePtr resp) {
//     // Get all connections from epoll
//     // For each connection (except exclude_fd), send message
    
//     std::cout << "[Broadcast] Message: " << resp->response_message 
//                 << " (exclude fd=" << resp->exclude_fd << ")\n";
    
//     // TODO: Need to iterate all connections
//     // This requires exposing connections from EpollInstance
// }