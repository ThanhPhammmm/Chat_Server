#include "JoinPublicChatRoomThreadHandler.h"

JoinPublicChatThreadHandler::JoinPublicChatThreadHandler(std::shared_ptr<JoinPublicChatHandler> handler,
                                                         std::shared_ptr<MessageQueue<HandlerRequestPtr>> req_queue,
                                                         std::shared_ptr<MessageQueue<HandlerResponsePtr>> resp_queue) 
    : BaseThreadHandler(handler, req_queue, resp_queue, "JoinPublicChatRoomHandler"),
      join_handler(handler) {}

void JoinPublicChatThreadHandler::run(){
    LOG_INFO_STREAM("┌────────────────────────────────────┐");
    LOG_INFO_STREAM("│ [JoinPublicChatRoomHandler] Started");
    LOG_INFO_STREAM("│ TID: " << std::this_thread::get_id());
    LOG_INFO_STREAM("└────────────────────────────────────┘");

    while(running.load()){
        auto req_opt = request_queue->pop(10);
        
        if(!req_opt.has_value()){
            if(!running.load()){
                break;
            }
            continue;
        }
        
        auto req = req_opt.value();
        
        std::string response = join_handler->handleMessage(req->connection, req->command);
        
        if(!response.empty()){
            auto resp = std::make_shared<HandlerResponse>();
            resp->connection = req->connection;
            resp->response_message = response;
            resp->fd = req->fd;
            resp->request_id = req->request_id;
            resp->is_public_room = false;
            resp->is_private_room = false;
            resp->is_private = true;  // Back to joining users
            resp->is_error = false;
            resp->is_broadcast = false;
            resp->is_list_users = false;
            resp->exclude_fd = -1;
            
            if(running.load()){
                response_queue->push(resp);
            }
        }
    }

    LOG_INFO_STREAM("[JoinPublicChatRoomHandler] Stopped");
}