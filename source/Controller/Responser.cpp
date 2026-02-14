#include "Responser.h"
#include "PublicChatRoom.h"
#include "Logger.h"
#include "MessageAckManager.h"
#include "UserManager.h"

Responser::Responser(std::shared_ptr<MessageQueue<HandlerResponsePtr>> resp_queue, EpollInstancePtr epoll)
    : response_queue(resp_queue),
      epoll_instance(epoll) {}

void Responser::start(){
    running.store(true);
    worker_thread = std::thread([this]() { run(); });
}

void Responser::stop(){
    running.store(false);
    if(response_queue){
        response_queue->stop();
    }
    if(worker_thread.joinable()){
        worker_thread.join();
    }
}

void Responser::run(){
    while(running.load()){
        auto resp_opt = response_queue->pop(100);        
        if(!resp_opt.has_value()){
            if(!running.load()){
                break;
            }
            continue;
        }
        
        auto resp = resp_opt.value();
        if(!resp){
            LOG_WARNING("Received null response");
            continue;
        }

        switch(resp->destination){
            case ResponseDestination::DIRECT_TO_CLIENT:
                sendToClient(resp);
                break;

            case ResponseDestination::ERROR_TO_CLIENT:
                sendBackToClient(resp);
                break;
                
            case ResponseDestination::BACK_TO_CLIENT:
                sendBackToClient(resp);
                break;
                
            case ResponseDestination::BROADCAST_PUBLIC_CHAT_ROOM:
                broadcastToRoom(resp);
                break;
                
            default:
                LOG_ERROR_STREAM("Unknown response destination: " << static_cast<int>(resp->destination));
                break;
        }
    }
}

bool Responser::trySend(int fd, const char* data, size_t len, size_t& sent){
    sent = 0;

    while(sent < len){
        ssize_t n = send(fd, data + sent, len - sent, MSG_NOSIGNAL | MSG_DONTWAIT);
        
        if(n < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                return true;
            }
            else if(errno == EPIPE || errno == ECONNRESET){
                LOG_DEBUG_STREAM("Connection closed during send (fd=" << fd << ")");
                return false;
            }
            else{
                LOG_ERROR_STREAM("Send error (fd=" << fd << "): " << strerror(errno));
                return false;
            }
        }
        else if(n == 0){
            LOG_WARNING_STREAM("Send returned 0 (fd=" << fd << ")");
            return false;
        }
        
        sent += n;
    }

    return false;
}

void Responser::sendWithEpoll(ConnectionPtr conn, int fd, const std::string& message){
    if(!conn || conn->isClosed()) {
        LOG_WARNING_STREAM("Cannot send to closed connection fd=" << fd);
        return;
    }
    
    if(!epoll_instance){
        LOG_ERROR("Epoll instance is null");
        return;
    }

    if(conn->isWriting()){
        conn->queueWrite(message);
        LOG_DEBUG_STREAM("Connection fd=" << fd << " already writing, queued " << message.size() << " bytes (queue size: " << conn->getWriteQueueSize() << ")");
        return;
    }

    size_t sent = 0;
    bool would_block = trySend(fd, message.data(), message.size(), sent);

    if(!would_block && sent == message.size()){
        LOG_DEBUG_STREAM("Sent complete message to fd=" << fd << " (" << message.size() << " bytes)");
        return;
    }

    if(sent < message.size()){
        std::string remaining(message.data() + sent, message.size() - sent);
        conn->setPartialWrite(std::move(remaining));
        conn->setWriting(true);
        
        LOG_DEBUG_STREAM("Partial send on fd=" << fd << ": sent=" << sent << "/" << message.size() << ", enabling EPOLLOUT");
        
        epoll_instance->enableWrite(fd, [this](int write_fd){
            this->handleWritable(write_fd);
        });
    }
    }

void Responser::handleWritable(int fd){
    auto conn = epoll_instance->getConnection(fd);
    if(!conn || conn->isClosed()){
        epoll_instance->disableWrite(fd);
        LOG_DEBUG_STREAM("Connection closed, disabled EPOLLOUT for fd=" << fd);
        return;
    }

    if(conn->hasPartialWrite()){
        std::string data = conn->getPartialWrite();
        size_t sent = 0;
        bool would_block = trySend(fd, data.data(), data.size(), sent);
        
        if(sent < data.size()){
            std::string remaining(data.data() + sent, data.size() - sent);
            conn->setPartialWrite(std::move(remaining));
            LOG_DEBUG_STREAM("EPOLLOUT fd=" << fd << ": still blocked, sent=" << sent << "/" << data.size());
            return;
        }
        
        LOG_DEBUG_STREAM("EPOLLOUT fd=" << fd << ": completed partial write (" << data.size() << " bytes)");
    }

    while(conn->hasWriteData()){
        std::string data = conn->popWriteData();
        if(data.empty()) break;
        
        size_t sent = 0;
        bool would_block = trySend(fd, data.data(), data.size(), sent);
        
        if(sent < data.size()){
            std::string remaining(data.data() + sent, data.size() - sent);
            conn->setPartialWrite(std::move(remaining));
            LOG_DEBUG_STREAM("EPOLLOUT fd=" << fd << ": blocked on queue item, sent=" << sent << "/" << data.size());
            return;
        }
        
        LOG_DEBUG_STREAM("EPOLLOUT fd=" << fd << ": sent queued message (" << data.size() << " bytes)");
    }

    conn->setWriting(false);
    epoll_instance->disableWrite(fd);
    LOG_DEBUG_STREAM("EPOLLOUT fd=" << fd << ": queue empty, disabled EPOLLOUT");
}

void Responser::sendToClient(HandlerResponsePtr resp){
    if(!resp || !resp->connection){
        LOG_WARNING("Attempted to send to null connection");
        return;
    }

    if(!epoll_instance){
        LOG_ERROR("Epoll instance is null");
        return;
    }

    auto& ackMgr = MessageAckManager::getInstance();
    auto& userMgr = UserManager::getInstance();
    
    std::string msg_id = ackMgr.generateMessageId();
    std::string full_message = msg_id + "|" + resp->response_message;

    int sender_id = -1;
    int receiver_id = -1;

    auto sender_user_id = userMgr.getUserId(resp->fd);
    LOG_DEBUG_STREAM("Sender fd=" << resp->fd << " maps to user_id=" << (sender_user_id.has_value() ? std::to_string(sender_user_id.value()) : "N/A"));
    if(sender_user_id.has_value()){
        sender_id = sender_user_id.value();
    }
    
    auto receiver_user_id = userMgr.getUserId(resp->user_destination);
    if(receiver_user_id.has_value()){
        receiver_id = receiver_user_id.value();
    }

    auto conn = resp->connection;
    int user_destination = resp->user_destination;

    if(!conn || conn->isClosed()){
        return;
    }

    int fd = conn->getFd();
    if(fd < 0){
        return;
    }

    ackMgr.addPendingMessage(msg_id, resp, conn, sender_id, receiver_id);

    sendWithEpoll(conn, user_destination, full_message);
}

void Responser::sendBackToClient(HandlerResponsePtr resp){
    if(!resp || !resp->connection){
        LOG_WARNING("Attempted to send to null connection");
        return;
    }
    
    if(!epoll_instance){
        LOG_ERROR("Epoll instance is null");
        return;
    }
    
    auto& ackMgr = MessageAckManager::getInstance();
    auto& userMgr = UserManager::getInstance();
    
    std::string msg_id = ackMgr.generateMessageId();
    std::string full_message = msg_id + "|" + resp->response_message;

    int sender_id = -1;
    auto sender_user_id = userMgr.getUserId(resp->fd);
    if(sender_user_id.has_value()){
        sender_id = sender_user_id.value();
    }

    auto conn = resp->connection;

    if(!conn || conn->isClosed()){
        return;
    }
    
    int fd = conn->getFd();
    if(fd < 0){
        return;
    }

    ackMgr.addPendingMessage(msg_id, resp, conn, sender_id, sender_id);

    sendWithEpoll(conn, fd, full_message);
}

void Responser::broadcastToRoom(HandlerResponsePtr resp){
    if(!resp){
        LOG_WARNING("Attempted to broadcast null response");
        return;
    }

    if(!epoll_instance){
        LOG_ERROR("Epoll instance is null");
        return;
    }

    auto& room = PublicChatRoom::getInstance();
    auto members = room.getParticipants();
    LOG_DEBUG_STREAM("[Broadcast] Sending to " << members.size() << " members in public chat room");
    
    int sent_count = 0;
    for(int member_fd : members){
        if(resp->exclude_fd >= 0 && member_fd == resp->exclude_fd){
            continue;
        }
        
        auto conn = epoll_instance->getConnection(member_fd);
        if(!conn || conn->isClosed()){
            room.leave(member_fd);
            continue;
        }
        
        sendWithEpoll(conn, member_fd, resp->response_message);
        sent_count++;
    }
    
    LOG_DEBUG_STREAM("[Broadcast] Sent to " << sent_count << " members in room");
}