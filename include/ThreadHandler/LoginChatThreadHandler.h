#pragma once

#include "BaseThreadHandler.h"
#include "LoginChatHandler.h"
#include "MessageQueue.h"
#include "MessageThreadHandler.h"
#include "Logger.h"

class LoginChatThreadHandler : public BaseThreadHandler{
    private:
        std::shared_ptr<LoginChatHandler> login_handler;
        
    protected:
        void run() override;
        
    public:
        LoginChatThreadHandler(std::shared_ptr<LoginChatHandler> handler,
                          std::shared_ptr<MessageQueue<HandlerRequestPtr>> req_queue,
                          std::shared_ptr<MessageQueue<HandlerResponsePtr>> resp_queue);
};

using LoginChatThreadHandlerPtr = std::shared_ptr<LoginChatThreadHandler>;