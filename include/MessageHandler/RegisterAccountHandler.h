#pragma once

#include "MessageHandler.h"
#include "DataBaseThread.h"

class RegisterAccountHandler : public MessageHandler{
    private:
        DataBaseThreadPtr db_thread;

    public:
        RegisterAccountHandler(DataBaseThreadPtr db_thread);
        std::string handleMessage(ConnectionPtr conn, CommandPtr command, EpollInstancePtr epoll_instance = nullptr) override;
};

using RegisterAccountHandlerPtr = std::shared_ptr<RegisterAccountHandler>;