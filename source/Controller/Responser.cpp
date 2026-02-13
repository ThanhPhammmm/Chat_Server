#include "Responser.h"
#include "PublicChatRoom.h"
#include "Logger.h"
#include "MessageAckManager.h"

Responser::Responser(std::shared_ptr<MessageQueue<HandlerResponsePtr>> resp_queue,
                     ThreadPoolPtr pool,
                     EpollInstancePtr epoll)
    : response_queue(resp_queue),
      thread_pool(pool),
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

void Responser::sendToClient(HandlerResponsePtr resp){
    if(!resp || !resp->connection){
        LOG_WARNING("Attempted to send to null connection");
        return;
    }

    auto& ackMgr = MessageAckManager::getInstance();
    std::string msg_id = ackMgr.generateMessageId();
    std::string full_message = msg_id + "|" + resp->response_message;

    thread_pool->submit([msg_id, resp, full_message](){
        if(!resp || !resp->connection){
            LOG_WARNING("Attempted to send to null connection");
            return;
        }
        auto conn = resp->connection;
        int user_destination = resp->user_destination;
        std::string content = resp->response_message;

        if(!conn || conn->isClosed()){
            return;
        }

        int fd = conn->getFd();
        if(fd < 0){
            return;
        }

        MessageAckManager::getInstance().addPendingMessage(msg_id, resp, conn);

        const char* data = full_message.data();
        size_t remaining = full_message.size();
        int retry_count = 0;
        const int MAX_RETRIES = 3;

        while(remaining > 0 && retry_count < MAX_RETRIES){
            if(conn->isClosed()) break;
            
            ssize_t sent = send(user_destination, data, remaining, MSG_NOSIGNAL);
            
            if(sent < 0){
                if(errno == EAGAIN || errno == EWOULDBLOCK){
                    usleep(1000);
                    retry_count++;
                    continue;
                }
                else if(errno == EPIPE || errno == ECONNRESET){
                    LOG_DEBUG_STREAM("Connection closed during send (fd=" << fd << ")");
                    break;
                }
                else{
                    LOG_ERROR_STREAM("Send error (fd=" << fd << "): " << strerror(errno));
                    break;
                }
                perror("send");
                break;
            }
            else if(sent == 0){
                LOG_WARNING_STREAM("Send returned 0 (fd=" << fd << ")");
                break;
            }
            else{
                data += sent;
                remaining -= sent;
                retry_count = 0;
            }
        }
        if(remaining > 0){
            LOG_WARNING_STREAM("Failed to send complete message (fd=" << fd << ", " << remaining << " bytes remaining)");
        }
    });
}

void Responser::sendBackToClient(HandlerResponsePtr resp){
    if(!resp || !resp->connection){
        LOG_WARNING("Attempted to send to null connection");
        return;
    }
    auto& ackMgr = MessageAckManager::getInstance();
    std::string msg_id = ackMgr.generateMessageId();
    std::string full_message = msg_id + "|" + resp->response_message;

    thread_pool->submit([msg_id, resp, full_message](){
        if(!resp || !resp->connection){
            LOG_WARNING("Attempted to send to null connection");
            return;
        }
        auto conn = resp->connection;
        std::string content = resp->response_message;

        if(!conn || conn->isClosed()){
            return;
        }        
        int fd = conn->getFd();
        if(fd < 0){
            return;
        }

        MessageAckManager::getInstance().addPendingMessage(msg_id, resp, conn);

        const char* data = content.data();
        size_t remaining = content.size();
        int retry_count = 0;
        const int MAX_RETRIES = 3;

        while(remaining > 0 && retry_count < MAX_RETRIES){
            if(conn->isClosed()) break;
            
            ssize_t sent = send(fd, data, remaining, MSG_NOSIGNAL);
            
            if(sent < 0){
                if(errno == EAGAIN || errno == EWOULDBLOCK){
                    usleep(1000);
                    retry_count++;
                    continue;
                }
                else if(errno == EPIPE || errno == ECONNRESET){
                    LOG_DEBUG_STREAM("Connection closed during send (fd=" << fd << ")");
                    break;
                }
                else{
                    LOG_ERROR_STREAM("Send error (fd=" << fd << "): " << strerror(errno));
                    break;
                }
                perror("send");
                break;
            }
            else if(sent == 0){
                LOG_WARNING_STREAM("Send returned 0 (fd=" << fd << ")");
                break;
            }
            else{
                data += sent;
                remaining -= sent;
                retry_count = 0;
            }
        }
        if(remaining > 0){
            LOG_WARNING_STREAM("Failed to send complete message (fd=" << fd << ", " << remaining << " bytes remaining)");
        }
    });
}

void Responser::broadcastToRoom(HandlerResponsePtr resp){
    if(!resp){
        LOG_WARNING("Attempted to broadcast null response");
        return;
    }

    PublicChatRoom tmp;
    auto& room = tmp.getInstance();
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
        
        thread_pool->submit([conn, content = resp->response_message, member_fd](){
            if(!conn || conn->isClosed()) return;
            
            const char* data = content.data();
            size_t remaining = content.size();
            int retry_count = 0;
            const int MAX_RETRIES = 3;

            while(remaining > 0 && retry_count < MAX_RETRIES){
                if(conn->isClosed()) break;
                
                ssize_t sent = send(member_fd, data, remaining, MSG_NOSIGNAL);
                
                if(sent < 0){
                    if(errno == EAGAIN || errno == EWOULDBLOCK){
                        usleep(1000);
                        retry_count++;
                        continue;
                    }
                    else if(errno == EPIPE || errno == ECONNRESET){
                        LOG_DEBUG_STREAM("[Broadcast] Connection closed (fd=" << member_fd << ")");
                        break;
                    }
                    else{
                        LOG_ERROR_STREAM("[Broadcast] Send error (fd=" << member_fd << "): " << strerror(errno));
                        break;
                    }
                }
                else if(sent == 0){
                    break;
                }
                else{
                    data += sent;
                    remaining -= sent;
                    retry_count = 0;
                }
            }
        });
        
        sent_count++;
    }
    
    LOG_DEBUG_STREAM("[Broadcast] Sent to " << sent_count << " members in room");
}