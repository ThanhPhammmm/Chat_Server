#pragma once

#include "MessageHandler.h"
#include "DataBaseThread.h"

class RegisterAccountHandler : public MessageHandler{
    private:
        DataBaseThreadPtr db_thread;

        /* Wait for database thread response */
        std::string result;
        std::mutex result_mutex;
        std::condition_variable result_cv;

        bool done = false;
    public:
        RegisterAccountHandler(DataBaseThreadPtr db_thread);
        std::string handleMessage(ConnectionPtr conn, CommandPtr command, EpollInstancePtr epoll_instance = nullptr) override;
};

using RegisterAccountHandlerPtr = std::shared_ptr<RegisterAccountHandler>;