#include "PublicChatThreadHandler.h"

PublicChatThreadHandler::PublicChatThreadHandler(std::shared_ptr<PublicChatHandler> handler,
                                                 std::shared_ptr<MessageQueue<HandlerRequestPtr>> req_queue,
                                                 std::shared_ptr<MessageQueue<HandlerResponsePtr>> resp_queue) 
    : BaseThreadHandler(handler, req_queue, resp_queue, "PublicChatHandler"),
      public_chat_handler(handler) {}

void PublicChatThreadHandler::run(){
    while(running.load()){
        auto req_opt = request_queue->pop(100);
        if(!req_opt.has_value()){
            if(!running.load()){
                break;
            }
            continue;
        }

        auto req = req_opt.value();
        if(!req || !req->connection){
            LOG_WARNING("Received null request or connection");
            continue;
        }

        std::string response = public_chat_handler->handleMessage(req->connection, req->command);
        
        if(!response.empty()){
            auto resp = std::make_shared<HandlerResponse>();
            resp->connection = req->connection;
            resp->response_message = response;
            resp->fd = req->fd;
            resp->request_id = req->request_id;
            resp->user_destination = -1;
            
            // Back to client
            if(response.find("Error:") == 0){
                resp->destination = ResponseDestination::ERROR_TO_CLIENT;
            }
            else{
                resp->destination = ResponseDestination::BROADCAST_PUBLIC_CHAT_ROOM;
            }
            resp->exclude_fd = -1;
            
            if(running.load()){
                response_queue->push(resp);
            }
        }
    }

    LOG_INFO_STREAM("[PublicChatThreadHandler] Stopped");
}