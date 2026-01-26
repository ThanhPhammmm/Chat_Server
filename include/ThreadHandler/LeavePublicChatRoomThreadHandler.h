#pragma once

#include "BaseThreadHandler.h"
#include "LeavePublicChatRoomHandler.h"
#include "MessageQueue.h"
#include "MessageThreadHandler.h"
#include "Logger.h"

class LeavePublicChatThreadHandler : public BaseThreadHandler{
    private:
        std::shared_ptr<LeavePublicChatHandler> leave_handler;
        
    protected:
        void run() override;
        
    public:
        LeavePublicChatThreadHandler(std::shared_ptr<LeavePublicChatHandler> handler,
                                     std::shared_ptr<MessageQueue<HandlerRequestPtr>> req_queue,
                                     std::shared_ptr<MessageQueue<HandlerResponsePtr>> resp_queue);
};

using LeavePublicChatHandlerThreadPtr = std::shared_ptr<LeavePublicChatThreadHandler>;