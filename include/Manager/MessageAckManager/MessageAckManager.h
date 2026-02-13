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
#include "DataBaseThread.h"

struct PendingMessage{
    std::string message_id;
    HandlerResponsePtr responser;
    ConnectionPtr connection;
    std::chrono::system_clock::time_point send_time;
    int retry_count;
    int max_retries;
    int sender_id;
    int receiver_id;
    
    PendingMessage() : retry_count(0), max_retries(3), sender_id(-1), receiver_id(-1) {}
};

class MessageAckManager{
    private:
        std::unordered_map<std::string, PendingMessage> pending_messages;
        std::mutex pending_mutex;
        std::atomic<uint64_t> message_id_counter{0};
        DataBaseThreadPtr db_thread;

        const int ACK_TIMEOUT_MS = 5000; // 5s
        
        MessageAckManager();
        ~MessageAckManager();
        
        MessageAckManager(const MessageAckManager&) = delete;
        MessageAckManager& operator=(const MessageAckManager&) = delete;

    public:
        static MessageAckManager& getInstance();

        void setDatabaseThread(DataBaseThreadPtr db_thread);

        std::string generateMessageId();
        void addPendingMessage(const std::string& msg_id, HandlerResponsePtr response, ConnectionPtr conn, int sender_id, int receiver_id);        
        void acknowledgeMessage(const std::string& msg_id);
        void checkTimeouts();
        void resendMessage(const PendingMessage& msg);

        void persistPendingMessage(const PendingMessage& msg);
        void updateMessageStatusInDB(const std::string& msg_id, const std::string& status);
        void removeMessageFromDB(const std::string& msg_id);
        void loadPendingMessagesFromDB();  // Load on startup
};