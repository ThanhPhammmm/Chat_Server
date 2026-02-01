#pragma once

#include "BaseThreadHandler.h"
#include "LogoutChatHandler.h"
#include "MessageQueue.h"
#include "MessageThreadHandler.h"
#include "Logger.h"

class LogoutChatThreadHandler : public BaseThreadHandler{
    private:
        std::shared_ptr<LogoutChatHandler> logout_handler;
        
    protected:
        void run() override;
        
    public:
        LogoutChatThreadHandler(std::shared_ptr<LogoutChatHandler> handler,
                                std::shared_ptr<MessageQueue<HandlerRequestPtr>> req_queue,
                                std::shared_ptr<MessageQueue<HandlerResponsePtr>> resp_queue);
};

using LogoutChatThreadHandlerPtr = std::shared_ptr<LogoutChatThreadHandler>;