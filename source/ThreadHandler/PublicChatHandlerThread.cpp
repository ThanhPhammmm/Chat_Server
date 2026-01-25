#include "PublicChatHandlerThread.h"

PublicChatHandlerThread::PublicChatHandlerThread(std::shared_ptr<PublicChatHandler> handler,
                                                 std::shared_ptr<MessageQueue<HandlerRequestPtr>> req_queue,
                                                 std::shared_ptr<MessageQueue<HandlerResponsePtr>> resp_queue) 
    : BaseThreadHandler(handler, req_queue, resp_queue, "PublicChatHandler"),
      public_chat_handler(handler) {}

void PublicChatHandlerThread::run(){
    LOG_INFO_STREAM("┌────────────────────────────────────┐");
    LOG_INFO_STREAM("│ [PublicChatHandler] Started");
    LOG_INFO_STREAM("│ TID: " << std::this_thread::get_id());
    LOG_INFO_STREAM("└────────────────────────────────────┘");

    while(running.load()){
        auto req_opt = request_queue->pop(100);
        
        if(!req_opt.has_value()) continue;
        
        auto req = req_opt.value();
        
        std::string response = public_chat_handler->handleMessage(req->connection, req->command);
        
        if(!response.empty()){
            auto resp = std::make_shared<HandlerResponse>();
            resp->connection = req->connection;
            resp->response_message = response;
            resp->fd = req->fd;
            resp->request_id = req->request_id;
            resp->is_public = true;
            resp->is_private = false;
            resp->is_broadcast = false;  // No broadcast for now
            resp->is_list_users = false;
            resp->exclude_fd = -1;
            
            response_queue->push(resp);
        }
    }

    LOG_INFO_STREAM("[PublicChatHandler] Stopped");
}