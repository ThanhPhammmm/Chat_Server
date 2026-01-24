#include "PublicChatHandlerThread.h"

PublicChatHandlerThread::PublicChatHandlerThread(std::shared_ptr<PublicChatHandler> public_chat_handler,
                                                 std::shared_ptr<MessageQueue<HandlerRequestPtr>> req_queue,
                                                 std::shared_ptr<MessageQueue<HandlerResponsePtr>> resp_queue) 
    : BaseThreadHandler(public_chat_handler,
                        req_queue,
                        resp_queue,
                        "PublicChatHandler"){}

void PublicChatHandlerThread :: run(){
    std::cout << "┌────────────────────────────────────┐\n";
    std::cout << "│ [PublicChatHandler] Started\n";
    std::cout << "│ TID: " << std::this_thread::get_id() << "\n";
    std::cout << "└────────────────────────────────────┘\n";

    while(running.load()){
        auto req_opt = request_queue->pop(100);
        
        if(!req_opt.has_value()) continue;
        
        auto req = req_opt.value();
        
        // Process request
        std::string response = public_chat_handler->handleMessage(req->connection, req->command);
        
        if(!response.empty()){
            auto resp = std::make_shared<HandlerResponse>();
            resp->connection = req->connection;
            resp->response_message = response;
            resp->fd = req->fd;
            resp->request_id = req->request_id;
            resp->is_broadcast = true; 
            resp->exclude_fd = req->fd;
            
            response_queue->push(resp);
        }
    }

    std::cout << "[PublicChatHandler] Stopped\n";
};