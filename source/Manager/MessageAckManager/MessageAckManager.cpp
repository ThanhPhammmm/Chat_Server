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

std::string MessageAckManager::generateMessageId(){
    uint64_t id = message_id_counter.fetch_add(1);
    std::ostringstream oss;
    oss << "MSG_" << std::setfill('0') << std::setw(10) << id;
    return oss.str();
}

void MessageAckManager::addPendingMessage(const std::string& msg_id, HandlerResponsePtr responser, ConnectionPtr conn){
    std::lock_guard<std::mutex> lock(pending_mutex);
    
    PendingMessage pending;
    pending.message_id = msg_id;
    pending.responser = responser;
    pending.connection = conn;
    pending.send_time = std::chrono::system_clock::now();
    pending.retry_count = 0;
    
    pending_messages[msg_id] = pending;
    
    LOG_DEBUG_STREAM("[ACK] Added pending message: " << msg_id << " for fd=" << responser->fd);
}

void MessageAckManager::acknowledgeMessage(const std::string& msg_id){
    std::lock_guard<std::mutex> lock(pending_mutex);
    
    auto it = pending_messages.find(msg_id);
    if(it != pending_messages.end()){
        LOG_DEBUG_STREAM("[ACK] Acknowledged message: " << msg_id << " for fd=" << it->second.responser->fd);
        pending_messages.erase(it);
    }
}

void MessageAckManager::resendMessage(const PendingMessage& msg){
    if(!msg.connection || msg.connection->isClosed()){
        LOG_WARNING_STREAM("[ACK] Cannot resend to closed connection fd=" << msg.responser->fd);
        return;
    }
    
    std::string full_msg = msg.message_id + "|" + msg.responser->response_message;
    
    const char* data = full_msg.data();
    size_t remaining = full_msg.size();
    
    while(remaining > 0){
        ssize_t sent = send(msg.responser->fd, data, remaining, MSG_NOSIGNAL);
        
        if(sent < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                usleep(1000);
                continue;
            }
            LOG_ERROR_STREAM("[ACK] Resend failed for fd=" << msg.responser->fd << ": " << strerror(errno));
            return;
        }
        
        data += sent;
        remaining -= sent;
    }
    
    LOG_INFO_STREAM("[ACK] Resent message " << msg.message_id << " to fd=" << msg.responser->fd << " (retry " << msg.retry_count + 1 << ")");
}

void MessageAckManager::checkTimeouts(){
    std::lock_guard<std::mutex> lock(pending_mutex);
    
    auto now = std::chrono::system_clock::now();
    std::vector<std::string> to_remove;
    
    for(auto& [msg_id, pending] : pending_messages){
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - pending.send_time
        ).count();
        
        if(elapsed >= ACK_TIMEOUT_MS){
            if(pending.retry_count >= pending.max_retries){
                LOG_WARNING_STREAM("[ACK] Message " << msg_id << " failed after " << pending.max_retries << " retries, giving up");
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
}

