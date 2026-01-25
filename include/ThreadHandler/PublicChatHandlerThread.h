#pragma once

#include "BaseThreadHandler.h"
#include "PublicChatHandler.h"
#include "ThreadMessageHandler.h"
#include "MessageQueue.h"
#include "Epoll.h"
#include "Logger.h"

class PublicChatHandlerThread : public BaseThreadHandler{
    private:
        std::shared_ptr<PublicChatHandler> public_chat_handler;
        
    // protected:
        void run();
        
    public:
        PublicChatHandlerThread(std::shared_ptr<PublicChatHandler> handler,
                                std::shared_ptr<MessageQueue<HandlerRequestPtr>> req_queue,
                                std::shared_ptr<MessageQueue<HandlerResponsePtr>> resp_queue);
};

using PublicChatHandlerThreadPtr = std::shared_ptr<PublicChatHandlerThread>;