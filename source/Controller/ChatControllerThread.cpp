#include "ChatControllerThread.h"
#include "Logger.h"
#include "UserManager.h"

ChatControllerThread::ChatControllerThread(std::shared_ptr<MessageQueue<Message>> incoming_queue, 
                                           std::shared_ptr<MessageQueue<HandlerResponsePtr>> response_queue,
                                           EpollInstancePtr epoll_instance)
    : incoming_queue(incoming_queue),
    response_queue(response_queue),
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
    auto cmd = parser.parse(content, incoming, epoll_instance);
    LOG_DEBUG_STREAM("[Router]: " << static_cast<int>(cmd->type));

    if(!cmd){
        LOG_ERROR("[Router] Parser returned null command");
        return;
    }

    auto it = handler_queues.find(cmd->type);
    if(it == handler_queues.end()){
        LOG_WARNING_STREAM("[Router] Unknown command type: " << static_cast<int>(cmd->type) << " from fd=" << incoming.fd);
        
        if(cmd->type == CommandType::UNKNOWN && incoming.connection && !incoming.connection->isClosed()){
            std::string error_msg = "Error: Unknown command. Available: "
                                    "/register <user> <pass> | "
                                    "/login <user> <pass> | "
                                    "/logout | "
                                    "/join_public_chat_room | "
                                    "/leave_public_chat_room | "
                                    "/list_online_users | "
                                    "/private_chat <user> <msg> | "
                                    "<msg> (sends to public chat)";
            
            auto resp = std::make_shared<HandlerResponse>();
            resp->connection = incoming.connection;
            resp->response_message = error_msg;
            resp->fd = incoming.fd;
            resp->destination = ResponseDestination::BACK_TO_CLIENT;
            resp->exclude_fd = -1;
            resp->user_destination = -1;
            
            if(response_queue){
                response_queue->push(resp);
                LOG_DEBUG_STREAM("[Router] Sent unknown command error to fd=" << incoming.fd);
            }
        }
        return;
    }
    
    auto req = std::make_shared<HandlerRequest>();
    req->connection = incoming.connection;
    req->command = cmd;
    req->fd = incoming.fd;

    if(cmd->type == CommandType::PRIVATE_CHAT && (cmd->args).size() > 1){
        auto& userMgr = UserManager::getInstance();
        auto target_fd = userMgr.getFd(cmd->args[0]);
        if(target_fd.has_value()){
            req->user_desntination = target_fd.value();
        } 
        else req->user_desntination = -1;

        LOG_DEBUG_STREAM("[Router] Routing private message to user " << cmd->args[0] << " with fd=" << req->user_desntination);
    }
    else{
        req->user_desntination = -1;
    }
    it->second->push(req);
    
    LOG_DEBUG_STREAM("[Router] Routed message of " << req->fd << " to handler for type " << static_cast<int>(cmd->type));
}