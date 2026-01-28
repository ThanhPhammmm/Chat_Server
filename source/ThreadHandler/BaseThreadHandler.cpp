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
    if(!running.compare_exchange_strong(expected, false, std::memory_order_acq_rel)){
        return;
    }
    
    if(request_queue){
        request_queue->stop();
    }
    
    if(worker_thread.joinable()){
        worker_thread.join();
    }
}