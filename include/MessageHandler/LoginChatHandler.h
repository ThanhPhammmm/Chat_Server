#pragma once

#include "MessageHandler.h"
#include "DataBaseThread.h"

class LoginChatHandler : public MessageHandler{
    private:
        DataBaseThreadPtr db_thread;

        /* Wait for database thread response */
        std::string result;
        std::mutex result_mutex;
        std::condition_variable result_cv;
        bool done = false;
        bool login_success = false;
        
    public:
        LoginChatHandler(DataBaseThreadPtr db_thread);
        std::string handleMessage(ConnectionPtr conn, CommandPtr command, EpollInstancePtr epoll_instance = nullptr) override;
};

using LoginChatHandlerPtr = std::shared_ptr<LoginChatHandler>;