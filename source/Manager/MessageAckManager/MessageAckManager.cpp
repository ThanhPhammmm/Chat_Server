#include <sstream>
#include <iomanip>
#include "Responser.h"
#include "MessageAckManager.h"
#include "Logger.h"

MessageAckManager::MessageAckManager() {}

MessageAckManager::~MessageAckManager(){}

MessageAckManager& MessageAckManager::getInstance(){
    static MessageAckManager instance;
    return instance;
}

void MessageAckManager::setDatabaseThread(DataBaseThreadPtr thread){
    db_thread = thread;
    LOG_INFO("MessageAckManager: Database thread set");
}

std::string MessageAckManager::generateMessageId(){
    uint64_t id = message_id_counter.fetch_add(1);
    std::ostringstream oss;
    oss << "MSG_" << std::setfill('0') << std::setw(10) << id;
    return oss.str();
}

void MessageAckManager::addPendingMessage(const std::string& msg_id, HandlerResponsePtr response, ConnectionPtr conn,int sender_id,int receiver_id){
    std::lock_guard<std::mutex> lock(pending_mutex);
    
    PendingMessage pending;
    pending.message_id = msg_id;
    pending.responser = response;
    pending.connection = conn;
    pending.send_time = std::chrono::system_clock::now();
    pending.retry_count = 0;
    pending.sender_id = sender_id;
    pending.receiver_id = receiver_id;
    
    pending_messages[msg_id] = pending;
    
    persistPendingMessage(pending);
    
    LOG_DEBUG_STREAM("[ACK] Added pending message: " << msg_id << " from user_id=" << sender_id << " to user_id=" << receiver_id);
}

void MessageAckManager::acknowledgeMessage(const std::string& msg_id){
    std::lock_guard<std::mutex> lock(pending_mutex);
    
    auto it = pending_messages.find(msg_id);
    if(it != pending_messages.end()){
        LOG_DEBUG_STREAM("[ACK] Acknowledged message: " << msg_id);
        updateMessageStatusInDB(msg_id, "acknowledged");
        pending_messages.erase(it);
        //removeMessageFromDB(msg_id);
    }
}

void MessageAckManager::resendMessage(const PendingMessage& msg){
    if(!msg.connection || msg.connection->isClosed()){
        LOG_WARNING_STREAM("[ACK] Cannot resend to closed connection, sender_id=" << msg.sender_id << " receiver_id=" << msg.receiver_id);
        
        updateMessageStatusInDB(msg.message_id, "failed");
        return;
    }
    
    if(!msg.responser){
        LOG_ERROR_STREAM("[ACK] Cannot resend - null response for " << msg.message_id);
        updateMessageStatusInDB(msg.message_id, "failed");
        return;
    }
    
    std::string full_msg = msg.message_id + "|" + msg.responser->response_message;
    
    int target_fd = (msg.responser->destination == ResponseDestination::DIRECT_TO_CLIENT) ? msg.responser->user_destination : msg.responser->fd;
    
    if(target_fd < 0){
        LOG_ERROR_STREAM("[ACK] Invalid target_fd for resend: " << msg.message_id);
        updateMessageStatusInDB(msg.message_id, "failed");
        return;
    }
    
    const char* data = full_msg.data();
    size_t remaining = full_msg.size();
    size_t total_sent = 0;

    int send_attempts = 0;
    const int MAX_SEND_ATTEMPTS = 10;

    while(remaining > 0 && send_attempts < MAX_SEND_ATTEMPTS ){
        ssize_t sent = send(target_fd, data + total_sent, remaining, MSG_NOSIGNAL | MSG_DONTWAIT);
        
        if(sent < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                send_attempts++;
                if(send_attempts >= MAX_SEND_ATTEMPTS){
                    LOG_ERROR_STREAM("[ACK] Resend blocked after " << MAX_SEND_ATTEMPTS << " attempts for " << msg.message_id);
                    updateMessageStatusInDB(msg.message_id, "failed");
                    return;
                }
                usleep(100);
                continue;
            }
            
            LOG_ERROR_STREAM("[ACK] Resend failed for " << msg.message_id << " to fd=" << target_fd << ": " << strerror(errno));
            updateMessageStatusInDB(msg.message_id, "failed");
            return;
        }

        total_sent += sent;
        remaining -= sent;
        send_attempts = 0;
    }
    
    if(remaining > 0){
        LOG_ERROR_STREAM("[ACK] Partial resend for " << msg.message_id << ", sent=" << total_sent << "/" << full_msg.size());
        updateMessageStatusInDB(msg.message_id, "failed");
        return;
    }

    LOG_INFO_STREAM("[ACK] Resent message " << msg.message_id << " to fd=" << target_fd << " (retry " << msg.retry_count + 1 << ")");
    
    if(db_thread){
        auto req = std::make_shared<DBRequest>();
        req->type = DBOperationType::UPDATE_MESSAGE_STATUS;
        req->message_id = msg.message_id;
        req->status = "sent";
        db_thread->submitRequest(req);
    }
}

void MessageAckManager::checkTimeouts(){
    std::lock_guard<std::mutex> lock(pending_mutex);
    
    auto now = std::chrono::system_clock::now();
    std::vector<std::string> to_remove;
    
    for(auto& [msg_id, pending] : pending_messages){
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - pending.send_time).count();
        
        if(elapsed >= ACK_TIMEOUT_MS){
            if(pending.retry_count >= pending.max_retries){
                LOG_WARNING_STREAM("[ACK] Message " << msg_id << " failed after " << pending.max_retries << " retries, marking as failed");
                updateMessageStatusInDB(msg_id, "failed");
                //removeMessageFromDB(msg_id);
                to_remove.push_back(msg_id);
            }
            else{
                LOG_WARNING_STREAM("[ACK] Message " << msg_id << " timeout, resending...");
                pending.retry_count++;
                pending.send_time = now;
                resendMessage(pending);
            }
        }
    }
    
    for(const auto& msg_id : to_remove){
        pending_messages.erase(msg_id);
    }
        
    static int check_count = 0;
    if(++check_count % 60 == 0){
        LOG_DEBUG_STREAM("[ACK] Stats: " << pending_messages.size() << " pending messages");
    }
}

void MessageAckManager::persistPendingMessage(const PendingMessage& msg){
    if(!db_thread){
        LOG_WARNING("[ACK] Cannot persist message - database thread not set");
        return;
    }
    
    auto req = std::make_shared<DBRequest>();
    req->type = DBOperationType::ADD_PENDING_MESSAGE;
    req->message_id = msg.message_id;
    req->sender_id = msg.sender_id;
    req->receiver_id = msg.receiver_id;

    // Capture msg_id by value to avoid dangling reference
    std::string msg_id_copy = msg.message_id;
    req->callback = [msg_id_copy](bool success, std::string& result){
        if(success){
            LOG_DEBUG_STREAM("[ACK] Persisted message to DB: " << msg_id_copy);
        }
        else{
            LOG_ERROR_STREAM("[ACK] Failed to persist message: " << msg_id_copy << " - " << result);
        }
    };
    
    db_thread->submitRequest(req);
}

void MessageAckManager::updateMessageStatusInDB(const std::string& msg_id, const std::string& status){
    if(!db_thread){
        return;
    }
    
    auto req = std::make_shared<DBRequest>();
    req->type = DBOperationType::UPDATE_MESSAGE_STATUS;
    req->message_id = msg_id;
    req->status = status;

    std::string msg_id_copy = msg_id;
    std::string status_copy = status;
    req->callback = [msg_id_copy, status_copy](bool success, std::string& result){
        if(success){
            LOG_DEBUG_STREAM("[ACK] Updated message " << msg_id_copy << " status to: " << status_copy);
        }
        else{
            LOG_ERROR_STREAM("[ACK] Failed to update status: " << result);
        }
    };
    
    db_thread->submitRequest(req);
}

void MessageAckManager::removeMessageFromDB(const std::string& msg_id){
    if(!db_thread){
        return;
    }
    
    auto req = std::make_shared<DBRequest>();
    req->type = DBOperationType::DELETE_PENDING_MESSAGE;
    req->message_id = msg_id;

    std::string msg_id_copy = msg_id;
    req->callback = [msg_id_copy](bool success, std::string& result){
        if(success){
            LOG_DEBUG_STREAM("[ACK] Removed message from DB: " << msg_id_copy);
        }
        else{
            LOG_ERROR_STREAM("[ACK] Failed to remove from DB: " << result);
        }
    };
    
    db_thread->submitRequest(req);
}

void MessageAckManager::loadPendingMessagesFromDB(){
    if(!db_thread){
        LOG_WARNING("[ACK] Cannot load pending messages - database thread not set");
        return;
    }
    
    LOG_INFO("[ACK] Loading pending messages from database...");
    
    auto req = std::make_shared<DBRequest>();
    req->type = DBOperationType::GET_PENDING_MESSAGES;
    req->callback = [this](bool success, std::string& result){
        if(success){
            LOG_INFO_STREAM("[ACK] " << result);
            // Note: Actual restoration of PendingMessage objects would require
            // mapping user IDs back to current connections, which may not exist yet
            // This is mainly for auditing/cleanup of stale messages
        }
        else{
            LOG_ERROR_STREAM("[ACK] Failed to load pending messages: " << result);
        }
    };
    
    db_thread->submitRequest(req);
}