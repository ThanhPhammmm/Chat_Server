#include "MessageAckManagerThreadHandler.h"
#include "MessageAckManager.h"
#include "Logger.h"
#include <chrono>

MessageAckThreadHandler::MessageAckThreadHandler() {}

MessageAckThreadHandler::~MessageAckThreadHandler(){
    stop();
}

void MessageAckThreadHandler::start(){
    running.store(true);
    worker_thread = std::thread([this](){ run(); });
    LOG_INFO("MessageAckThreadHandler started");
}

void MessageAckThreadHandler::stop(){
    running.store(false);
    if(worker_thread.joinable()){
        worker_thread.join();
    }
    LOG_INFO("MessageAckThreadHandler stopped");
}

void MessageAckThreadHandler::run(){
    auto& ackMgr = MessageAckManager::getInstance();
    
    while(running.load()){
        ackMgr.checkTimeouts();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}