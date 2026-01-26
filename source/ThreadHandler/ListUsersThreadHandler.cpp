#include "ListUsersThreadHandler.h"

ListUsersThreadHandler::ListUsersThreadHandler(std::shared_ptr<ListUsersHandler> handler,
                                               std::shared_ptr<MessageQueue<HandlerRequestPtr>> req_queue,
                                               std::shared_ptr<MessageQueue<HandlerResponsePtr>> resp_queue,
                                               std::shared_ptr<EpollInstance> epoll_instance)
    : BaseThreadHandler(handler, req_queue, resp_queue, "ListUsersHandler"),
      list_users_handler(handler),
      epoll_instance(epoll_instance) {}

void ListUsersThreadHandler::run(){
    LOG_INFO_STREAM("┌────────────────────────────────────┐");
    LOG_INFO_STREAM("│   [ListUsersHandler] Started       │");
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
        
        std::string response = list_users_handler->handleMessage(req->connection, req->command, epoll_instance);
        
        if(!response.empty()){
            auto resp = std::make_shared<HandlerResponse>();
            resp->connection = req->connection;
            resp->response_message = response;
            resp->fd = req->fd;
            resp->request_id = req->request_id;
            resp->is_public_room = false;
            resp->is_private_room = false;
            resp->is_broadcast = false;  // No broadcast for now
            resp->is_list_users = true;
            resp->exclude_fd = -1;
            
            if(running.load()){
                response_queue->push(resp);
            }
        }
    }
    LOG_INFO_STREAM("[ListUsersHandler] Stopped");
}