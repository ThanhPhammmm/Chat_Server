#pragma once

#include "BaseThreadHandler.h"
#include "PrivateChatHandler.h"
#include "MessageQueue.h"
#include "MessageThreadHandler.h"
#include "Epoll.h"
#include "Logger.h"

class PrivateChatThreadHandler : public BaseThreadHandler{
    private:
        std::shared_ptr<PrivateChatHandler> private_chat_handler;
        std::shared_ptr<EpollInstance> epoll_instance;
    protected:
        void run() override;

    public:
        PrivateChatThreadHandler(std::shared_ptr<PrivateChatHandler> handler,
                                 std::shared_ptr<MessageQueue<HandlerRequestPtr>> req_queue,
                                 std::shared_ptr<MessageQueue<HandlerResponsePtr>> resp_queue,
                                 std::shared_ptr<EpollInstance> epoll_instance);
};

using PrivateChatHandlerThreadPtr = std::shared_ptr<PrivateChatThreadHandler>;