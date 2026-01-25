#include "ChatControllerThread.h"
#include "TCPServer.h"

ChatControllerThread::ChatControllerThread(std::shared_ptr<MessageQueue<Message>> incoming_queue)
    : incoming_queue(incoming_queue) {}

void ChatControllerThread::registerHandlerQueue(CommandType type, std::shared_ptr<MessageQueue<HandlerRequestPtr>> queue){
    handler_queues[type] = queue;
}

void ChatControllerThread::start(){
    running.store(true);
    worker_thread = std::thread([this]() { run(); });
}   

void ChatControllerThread::stop(){
    running.store(false);
    incoming_queue->stop();
    for(auto& pair : handler_queues){
        pair.second->stop();
    }
    if(worker_thread.joinable()){
        worker_thread.join();
    }
}

void ChatControllerThread::run(){
        LOG_INFO_STREAM("┌────────────────────────────────────┐");
        LOG_INFO_STREAM("│   [ChatControllerThread] Started   │");
        LOG_INFO_STREAM("│ TID: " << std::this_thread::get_id());
        LOG_INFO_STREAM("└────────────────────────────────────┘");

    CommandParser parser;

    while(running.load()){
        auto msg_opt = incoming_queue->pop(100);
        
        if(!msg_opt.has_value()) continue;
        
        auto msg = msg_opt.value();
        
        switch(msg.type){
            case MessageType::INCOMING_MESSAGE:
                routeMessage(std::get<IncomingMessage>(msg.payload), parser);
                break;
                
            // case MessageType::CLIENT_DISCONNECTED:
            //     handleDisconnect(std::get<ClientDisconnected>(msg.payload));
            //     break;
                
            case MessageType::SHUTDOWN:
                running.store(false);
                break;
                
            default:
                break;
        }
    }

    LOG_INFO_STREAM("[ChatControllerThread] Stopped");}

void ChatControllerThread::routeMessage(const IncomingMessage& incoming, CommandParser& parser){
    std::string content = incoming.content;
    auto cmd = parser.parse(content);
    
    auto it = handler_queues.find(cmd->type);
    if(it == handler_queues.end()){
        LOG_WARNING_STREAM("[Router] Unknown command type: " << static_cast<int>(cmd->type));        
        return;
    }
    
    auto req = std::make_shared<HandlerRequest>();
    req->connection = incoming.connection;
    req->command = cmd;
    req->fd = incoming.fd;
    req->request_id = request_counter.fetch_add(1);
    
    it->second->push(req);
    
    LOG_DEBUG_STREAM("[Router] Routed request #"
                    << req->request_id
                    << " to handler for type "
                    << static_cast<int>(cmd->type));
}

// void ChatControllerThread::handleDisconnect(const ClientDisconnected& disc){
//     int fd = disc.fd;
//     std::cout << "[Router] Handling disconnect for fd: " << fd << "\n";
//     // Additional cleanup logic can be added here
// }