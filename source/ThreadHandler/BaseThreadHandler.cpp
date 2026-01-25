#include "BaseThreadHandler.h"

BaseThreadHandler::BaseThreadHandler(MessageHandlerPtr message_handler,
                                     std::shared_ptr<MessageQueue<HandlerRequestPtr>> request_queue,
                                     std::shared_ptr<MessageQueue<HandlerResponsePtr>> response_queue,
                                     const std::string& handler_name)
    : message_handler(message_handler),
      request_queue(request_queue),
      response_queue(response_queue),
      handler_name(handler_name) {}

BaseThreadHandler::~BaseThreadHandler(){
    stop();
}

void BaseThreadHandler::start(){
    running.store(true);
    worker_thread = std::thread([this]() { run(); });
}

void BaseThreadHandler::stop(){
    bool expected = true;
    if(!running.compare_exchange_strong(expected, false)){
        return;
    }

    if(request_queue){
        request_queue->stop();
    }

    if(worker_thread.joinable() &&
        std::this_thread::get_id() != worker_thread.get_id()){
        worker_thread.join();
    }
}


// void BaseThreadHandler::run(){
//     std::cout << "┌────────────────────────────────────┐\n";
//     std::cout << "│ [" << handler_name << "] Started\n";
//     std::cout << "│ TID: " << std::this_thread::get_id() << "\n";
//     std::cout << "└────────────────────────────────────┘\n";

//     while(running.load()){
//         auto req_opt = request_queue->pop(100);
        
//         if(!req_opt.has_value()) continue;
        
//         auto req = req_opt.value();
        
//         std::string response = message_handler->handleMessage(req->connection, req->command);
        
//         if(!response.empty()){
//             auto resp = std::make_shared<HandlerResponse>();
//             resp->connection = req->connection;
//             resp->response_message = response;
//             resp->fd = req->fd;
//             resp->request_id = req->request_id;
//             resp->is_broadcast = false;
//             resp->is_public = false;
//             resp->is_private = false;
//             resp->exclude_fd = -1;
            
//             response_queue->push(resp);
//         }
//     }

//     std::cout << "[" << handler_name << "] Stopped\n";
// }