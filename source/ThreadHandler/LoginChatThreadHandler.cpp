#include "LoginChatThreadHandler.h"

LoginChatThreadHandler::LoginChatThreadHandler(std::shared_ptr<LoginChatHandler> handler,
                                               std::shared_ptr<MessageQueue<HandlerRequestPtr>> req_queue,
                                               std::shared_ptr<MessageQueue<HandlerResponsePtr>> resp_queue)
    : BaseThreadHandler(handler, req_queue, resp_queue, "LoginChatHandler"),
      login_handler(handler) {}

void LoginChatThreadHandler::run(){
    while(running.load()){
        auto req_opt = request_queue->pop(100);
        if(!req_opt.has_value()){
            if(!running.load()) break;
            continue;
        }
        
        auto req = req_opt.value();
        if(!req || !req->connection){
            LOG_WARNING("Received null request or connection");
            continue;
        }
        
        std::string response = login_handler->handleMessage(req->connection, req->command);
        
        if(!response.empty()){
            auto resp = std::make_shared<HandlerResponse>();
            resp->connection = req->connection;
            resp->response_message = response;
            resp->fd = req->fd;
            resp->destination = ResponseDestination::BACK_TO_CLIENT;
            resp->exclude_fd = -1;
            resp->user_destination = -1;
            
            if(running.load()){
                response_queue->push(resp);
            }
        }
    }
    LOG_INFO_STREAM("[LoginChatThreadHandler] Stopped");
}