#pragma once

#include "MessageQueue.h"
#include "Message.h"
#include "ThreadMessageHandler.h"
#include "CommandParser.h"

class ChatControllerThread {
    private:
        std::shared_ptr<MessageQueue<Message>> incoming_queue;
        // Queues to each handler
        std::unordered_map<CommandType, std::shared_ptr<MessageQueue<HandlerRequestPtr>>> handler_queues;
        std::thread worker_thread;
        std::atomic<bool> running{false};
        std::atomic<int> request_counter{0};

        void run();
        void routeMessage(const IncomingMessage& incoming, CommandParser& parser);
        void handleDisconnect(const ClientDisconnected& disc);

    public:
        ChatControllerThread(std::shared_ptr<MessageQueue<Message>> incoming_queue);
        void registerHandlerQueue(CommandType type, std::shared_ptr<MessageQueue<HandlerRequestPtr>> queue);
        void start();
        void stop();

};

using ChatControllerThreadPtr = std::shared_ptr<ChatControllerThread>;
