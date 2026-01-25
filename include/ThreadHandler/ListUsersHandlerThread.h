#pragma once

#include "BaseThreadHandler.h"
#include "ListUsersHandler.h"
#include "MessageQueue.h"
#include "ThreadMessageHandler.h"
#include "Epoll.h"
#include "Logger.h"

class ListUsersHandlerThread : public BaseThreadHandler{
    private:
        std::shared_ptr<ListUsersHandler> list_users_handler;
        std::shared_ptr<EpollInstance> epoll_instance;
    protected:
        void run();

    public:
        ListUsersHandlerThread(std::shared_ptr<ListUsersHandler> handler,
                               std::shared_ptr<MessageQueue<HandlerRequestPtr>> req_queue,
                               std::shared_ptr<MessageQueue<HandlerResponsePtr>> resp_queue,
                               std::shared_ptr<EpollInstance> epoll_instance);
};

using ListUsersHandlerThreadPtr = std::shared_ptr<ListUsersHandlerThread>;