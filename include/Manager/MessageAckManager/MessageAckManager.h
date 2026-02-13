#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <memory>
#include <atomic>
#include <thread>
#include "Connection.h"
#include "Responser.h"

struct PendingMessage{
    std::string message_id;
    HandlerResponsePtr responser;
    ConnectionPtr connection;
    std::chrono::system_clock::time_point send_time;
    int retry_count;
    int max_retries;
    
    PendingMessage() : retry_count(0), max_retries(3) {}
};

class MessageAckManager{
    private:
        std::unordered_map<std::string, PendingMessage> pending_messages;
        std::mutex pending_mutex;
        std::atomic<uint64_t> message_id_counter{0};

        const int ACK_TIMEOUT_MS = 5000; // 5s
        
        MessageAckManager();
        ~MessageAckManager();
        
        MessageAckManager(const MessageAckManager&) = delete;
        MessageAckManager& operator=(const MessageAckManager&) = delete;

    public:
        static MessageAckManager& getInstance();
        
        std::string generateMessageId();
        void addPendingMessage(const std::string& msg_id, HandlerResponsePtr responser, ConnectionPtr conn);
        void acknowledgeMessage(const std::string& msg_id);
        void checkTimeouts();
        void resendMessage(const PendingMessage& msg);
};