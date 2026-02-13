#pragma once

#include "MessageHandler.h"
#include "DataBaseThread.h"

class LoginChatHandler : public MessageHandler{
    private:
        DataBaseThreadPtr db_thread;
         
    public:
        LoginChatHandler(DataBaseThreadPtr db_thread);
        std::string handleMessage(ConnectionPtr conn, CommandPtr command, EpollInstancePtr epoll_instance = nullptr) override;
};

using LoginChatHandlerPtr = std::shared_ptr<LoginChatHandler>;