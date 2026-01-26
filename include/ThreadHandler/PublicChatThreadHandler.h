#pragma once

#include "BaseThreadHandler.h"
#include "PublicChatHandler.h"
#include "MessageThreadHandler.h"
#include "MessageQueue.h"
#include "Epoll.h"
#include "Logger.h"

class PublicChatThreadHandler : public BaseThreadHandler{
    private:
        std::shared_ptr<PublicChatHandler> public_chat_handler;
        
    protected:
        void run() override;
        
    public:
        PublicChatThreadHandler(std::shared_ptr<PublicChatHandler> handler,
                                std::shared_ptr<MessageQueue<HandlerRequestPtr>> req_queue,
                                std::shared_ptr<MessageQueue<HandlerResponsePtr>> resp_queue);
};

using PublicChatHandlerThreadPtr = std::shared_ptr<PublicChatThreadHandler>;