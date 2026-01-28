#include "ChatControllerThread.h"
#include "Logger.h"

ChatControllerThread::ChatControllerThread(std::shared_ptr<MessageQueue<Message>> incoming_queue, EpollInstancePtr epoll_instance)
    : incoming_queue(incoming_queue),
      epoll_instance(epoll_instance) {}

void ChatControllerThread::registerHandlerQueue(CommandType type, std::shared_ptr<MessageQueue<HandlerRequestPtr>> queue){
    handler_queues[type] = queue;
}

void ChatControllerThread::start(){
    running.store(true);
    worker_thread = std::thread([this]() { run(); });
}   

void ChatControllerThread::stop(){
    running.store(false);
    if(incoming_queue){
        incoming_queue->stop();
    }
    for(auto& pair : handler_queues){
        pair.second->stop();
    }
    if(worker_thread.joinable()){
        worker_thread.join();
    }
}

void ChatControllerThread::run(){
    CommandParser parser;
    while(running.load()){
        auto msg_opt = incoming_queue->pop(100);        
        if(!msg_opt.has_value()){
            if(!running.load()){
                break;
            }
            continue;
        }

        auto msg = msg_opt.value();
        switch(msg.type){
            case MessageType::INCOMING_MESSAGE:
                if(running.load()){
                    routeMessage(std::get<IncomingMessage>(msg.payload), parser);
                }
                break;
                
            case MessageType::SHUTDOWN:
                running.store(false);
                break;
                
            default:
                LOG_WARNING_STREAM("[Router] Unknown message type: " << static_cast<int>(msg.type));
                break;
        }
    }
}

void ChatControllerThread::routeMessage(IncomingMessage& incoming, CommandParser& parser){
    if(!incoming.connection){
        LOG_WARNING("[Router] Received message with null connection");
        return;
    }
    
    if(incoming.connection->isClosed()){
        LOG_DEBUG_STREAM("[Router] Connection already closed for fd=" << incoming.fd);
        return;
    }

    std::string content = incoming.content;
    auto cmd = parser.parse(content, incoming,epoll_instance);
    LOG_DEBUG_STREAM("[Router]: " << static_cast<int>(cmd->type));

    if(!cmd){
        LOG_ERROR("[Router] Parser returned null command");
        return;
    }

    auto it = handler_queues.find(cmd->type);
    if(it == handler_queues.end()){
        LOG_WARNING_STREAM("[Router] Unknown command type: " << static_cast<int>(cmd->type) << " from fd=" << incoming.fd);
        
        if(cmd->type == CommandType::UNKNOWN && incoming.connection && !incoming.connection->isClosed()){
            std::string error_msg = "Error: Unknown command. Available commands:\n"
                                   "  /join_public_chat_room  - Join public chat\n"
                                   "  /leave_public_chat_room - Leave public chat room\n"
                                   "  /list_online_users     - List connected users\n"
                                   "  /login <name>           - Login (not implemented)\n"
                                   "  /logout                 - Logout (not implemented)\n"
                                   "  /private_chat <user>    - Private message (not implemented)\n"
                                   "  <message>               - Send to public chat (must join first)\n";
            
            int fd = incoming.fd;
            if(fd >= 0){
                send(fd, error_msg.c_str(), error_msg.size(), MSG_NOSIGNAL);
            }
        }
        return;
    }
    
    auto req = std::make_shared<HandlerRequest>();
    req->connection = incoming.connection;
    req->command = cmd;
    req->fd = incoming.fd;
    req->request_id = request_counter.fetch_add(1);
    if(cmd->type == CommandType::PRIVATE_CHAT){
        req->user_desntination = std::stoi(cmd->args[0]);
    }
    else req->user_desntination = -1;
    it->second->push(req);
    
    LOG_DEBUG_STREAM("[Router] Routed request #" << req->request_id << " to handler for type " << static_cast<int>(cmd->type));
}