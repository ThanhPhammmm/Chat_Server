#pragma once

#include "BaseThreadHandler.h"
#include "ListUsersHandler.h"
#include "MessageQueue.h"
#include "MessageThreadHandler.h"
#include "Epoll.h"
#include "Logger.h"

class ListUsersThreadHandler : public BaseThreadHandler{
    private:
        std::shared_ptr<ListUsersHandler> list_users_handler;
        std::shared_ptr<EpollInstance> epoll_instance;
    protected:
        void run() override;

    public:
        ListUsersThreadHandler(std::shared_ptr<ListUsersHandler> handler,
                               std::shared_ptr<MessageQueue<HandlerRequestPtr>> req_queue,
                               std::shared_ptr<MessageQueue<HandlerResponsePtr>> resp_queue,
                               std::shared_ptr<EpollInstance> epoll_instance);
};

using ListUsersHandlerThreadPtr = std::shared_ptr<ListUsersThreadHandler>;