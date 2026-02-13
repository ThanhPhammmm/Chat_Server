#include "ListUsersThreadHandler.h"

ListUsersThreadHandler::ListUsersThreadHandler(std::shared_ptr<ListUsersHandler> handler,
                                               std::shared_ptr<MessageQueue<HandlerRequestPtr>> req_queue,
                                               std::shared_ptr<MessageQueue<HandlerResponsePtr>> resp_queue,
                                               std::shared_ptr<EpollInstance> epoll_instance)
    : BaseThreadHandler(handler, req_queue, resp_queue, "ListUsersHandler"),
      list_users_handler(handler),
      epoll_instance(epoll_instance) {}

void ListUsersThreadHandler::run(){
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

        std::string response = list_users_handler->handleMessage(req->connection, req->command, epoll_instance);
        
        if(!response.empty()){
            auto resp = std::make_shared<HandlerResponse>();
            resp->connection = req->connection;
            resp->response_message = response;
            resp->fd = req->fd;
            resp->destination = response.find("Error:") == 0 ? 
                               ResponseDestination::ERROR_TO_CLIENT: 
                               ResponseDestination::BACK_TO_CLIENT;
            resp->exclude_fd = -1;
            resp->user_destination = -1;

            if(running.load()){
                response_queue->push(resp);
            }
        }
    }
    LOG_INFO_STREAM("[ListUsersThreadHandler] Stopped");
}