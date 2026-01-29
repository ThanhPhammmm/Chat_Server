#pragma once

#include "BaseThreadHandler.h"
#include "RegisterAccountHandler.h"
#include "MessageHandler.h"

class RegisterAccountThreadHandler : public BaseThreadHandler{
    private:
        std::shared_ptr<RegisterAccountHandler> register_account_handler;
        
    protected:
        void run() override;
        
    public:
        RegisterAccountThreadHandler(std::shared_ptr<RegisterAccountHandler> handler,
                                std::shared_ptr<MessageQueue<HandlerRequestPtr>> req_queue,
                                std::shared_ptr<MessageQueue<HandlerResponsePtr>> resp_queue);
};

using RegisterAccountHandlerPtr = std::shared_ptr<RegisterAccountHandler>;