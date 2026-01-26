#pragma once

#include "BaseThreadHandler.h"
#include "JoinPublicChatRoomHandler.h"
#include "MessageQueue.h"
#include "MessageThreadHandler.h"
#include "Logger.h"

class JoinPublicChatThreadHandler : public BaseThreadHandler{
    private:
        std::shared_ptr<JoinPublicChatHandler> join_handler;
        
    protected:
        void run() override;
        
    public:
        JoinPublicChatThreadHandler(std::shared_ptr<JoinPublicChatHandler> handler,
                                    std::shared_ptr<MessageQueue<HandlerRequestPtr>> req_queue,
                                    std::shared_ptr<MessageQueue<HandlerResponsePtr>> resp_queue);
};

using JoinPublicChatHandlerThreadPtr = std::shared_ptr<JoinPublicChatThreadHandler>;