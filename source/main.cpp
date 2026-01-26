#include "TCPServer.h"

EpollInstancePtr g_epoll = nullptr;
std::shared_ptr<MessageQueue<Message>> g_incoming = nullptr;
std::atomic<bool> g_shutdown_requested{false};

void signalHandler(int signum){
    g_shutdown_requested.store(true, std::memory_order_release);
    
    if(g_epoll){
        g_epoll->stop();
    }
    
    if(g_incoming){
        Message msg;
        msg.type = MessageType::SHUTDOWN;
        g_incoming->push(std::move(msg));
    }
}

int main(){
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler); 
    signal(SIGPIPE, SIG_IGN);

    // Initialize Logger
    auto& logger = Logger::getInstance();
    logger.setLogLevel(LogLevel::INFO);
    logger.setConsoleOutput(true);
    logger.setFileOutput(true);
    logger.setLogFile("Record/chat_server.log");
    
    LOG_INFO("╔════════════════════════════════════════════╗");
    LOG_INFO("║          Multi-Threaded Chat Server        ║");
    LOG_INFO("╚════════════════════════════════════════════╝");
    LOG_INFO_STREAM("Main Thread: " << std::this_thread::get_id());

    try{
        // 1. CREATE CORE COMPONENTS
        LOG_DEBUG("Creating core components...");
        auto thread_pool = std::make_shared<ThreadPool>(4);
        LOG_DEBUG("ThreadPool created with 4 workers");
        
        auto epoll_instance = std::make_shared<EpollInstance>();
        g_epoll = epoll_instance;
        LOG_DEBUG("EpollInstance created");
        
        // 2. CREATE MESSAGE QUEUES
        LOG_DEBUG("Creating message queues...");
        auto incoming_queue = std::make_shared<MessageQueue<Message>>();
        g_incoming = incoming_queue;
        
        auto to_public_queue = std::make_shared<MessageQueue<HandlerRequestPtr>>();
        auto to_list_users_queue = std::make_shared<MessageQueue<HandlerRequestPtr>>();
        auto to_join_queue = std::make_shared<MessageQueue<HandlerRequestPtr>>();
        auto to_leave_queue = std::make_shared<MessageQueue<HandlerRequestPtr>>();
        auto response_queue = std::make_shared<MessageQueue<HandlerResponsePtr>>();
        LOG_DEBUG("Message queues created");
        
        // 3. CREATE HANDLERS
        LOG_DEBUG("Creating message handlers...");
        auto public_handler = std::make_shared<PublicChatHandler>();
        LOG_DEBUG("PublicChatHandler created");
        
        LOG_DEBUG("Creating ListUsersHandler...");
        auto list_users_handler = std::make_shared<ListUsersHandler>();
        LOG_DEBUG("ListUsersHandler created");

        LOG_DEBUG("Creating JoinPublicChatHandler...");
        auto join_handler = std::make_shared<JoinPublicChatHandler>();
        LOG_DEBUG("JoinPublicChatHandler created");

        LOG_DEBUG("Creating LeavePublicChatHandler...");
        auto leave_handler = std::make_shared<LeavePublicChatHandler>();
        LOG_DEBUG("LeavePublicChatHandler created");

        // 4. CREATE HANDLER THREADS
        LOG_DEBUG("Creating PublicChatThreadHandler...");
        auto public_thread = std::make_shared<PublicChatThreadHandler>(public_handler, to_public_queue, response_queue);
        LOG_DEBUG("PublicChatThreadHandler created");
        
        LOG_DEBUG("Creating ListUsersThreadHandler...");
        auto list_users_thread = std::make_shared<ListUsersThreadHandler>(list_users_handler, to_list_users_queue, response_queue, epoll_instance);
        LOG_DEBUG("ListUsersThreadHandler created");

        LOG_DEBUG("Creating JoinPublicChatThreadHandler...");
        auto join_thread = std::make_shared<JoinPublicChatThreadHandler>(join_handler, to_join_queue, response_queue);
        LOG_DEBUG("JoinPublicChatThreadHandler created");

        LOG_DEBUG("Creating LeavePublicChatThreadHandler...");
        auto leave_thread = std::make_shared<LeavePublicChatThreadHandler>(leave_handler, to_leave_queue, response_queue);
        LOG_DEBUG("LeavePublicChatThreadHandler created");

        LOG_DEBUG("Creating router thread...");
        auto router = std::make_shared<ChatControllerThread>(incoming_queue, epoll_instance);
        router->registerHandlerQueue(CommandType::PUBLIC_CHAT, to_public_queue);
        router->registerHandlerQueue(CommandType::LIST_USERS, to_list_users_queue);
        router->registerHandlerQueue(CommandType::JOIN_PUBLIC_CHAT_ROOM, to_join_queue);
        router->registerHandlerQueue(CommandType::LEAVE_PUBLIC_CHAT_ROOM, to_leave_queue);
        LOG_DEBUG("ChatControllerThread created and configured");
        
        // 6. CREATE RESPONSE DISPATCHER
        LOG_DEBUG("Creating response dispatcher...");
        auto response_dispatcher = std::make_shared<Responser>(response_queue, thread_pool, epoll_instance);
        LOG_DEBUG("Responser created");
        
        // 7. CREATE TCP SERVER
        LOG_DEBUG("Creating TCP server...");
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(8080);
        
        auto server = std::make_shared<TCPServer>(addr, epoll_instance, thread_pool, incoming_queue
        );
        
        server->startServer();
        LOG_INFO("TCP Server started successfully");
        
        // 8. START ALL THREADS
        LOG_INFO("Starting worker threads...");
        
        public_thread->start();
        LOG_INFO("PublicChatThreadHandler started");
        
        list_users_thread->start();
        LOG_INFO("ListUsersThreadHandler started");

        join_thread->start();
        LOG_INFO("JoinPublicChatThreadHandler started");
        
        leave_thread->start();
        LOG_INFO("LeavePublicChatThreadHandler started");

        router->start();
        LOG_INFO("Router Thread started");
        
        response_dispatcher->start();
        LOG_INFO("Response Dispatcher started");
        
        EpollThread epoll_thread(epoll_instance);
        epoll_thread.start();
        LOG_INFO("Epoll Thread started");
        
        LOG_INFO("╔════════════════════════════════════╗");
        LOG_INFO("║   Server Ready on port 8080        ║");
        LOG_INFO("║   <message> - public chat          ║");
        LOG_INFO("║   Press Ctrl+C to stop             ║");
        LOG_INFO("╚════════════════════════════════════╝");
        
        // 9. MAIN THREAD: MONITORING
        int monitor_count = 0;
        while(!g_epoll->isStopped() && !g_shutdown_requested.load()){
            std::this_thread::sleep_for(std::chrono::seconds(5));
            
            size_t in_size = incoming_queue->size();
            size_t pub_size = to_public_queue->size();
            size_t resp_size = response_queue->size();
            
            LOG_DEBUG_STREAM("[STATS #" << ++monitor_count << "] "
                           << "Incoming:" << in_size << " "
                           << "Public:" << pub_size << " "
                           << "Response:" << resp_size);
            
            // Warning if queues are getting full
            if(in_size > 50 || pub_size > 50 || resp_size > 50){
                LOG_WARNING_STREAM("High queue usage detected - "
                                 << "In:" << in_size << " "
                                 << "Pub:" << pub_size << " "
                                 << "Resp:" << resp_size);
            }
        }
        
        // 10. GRACEFUL SHUTDOWN
        LOG_INFO("Initiating graceful shutdown...");
        
        epoll_thread.stop();
        LOG_INFO("Epoll stopped");
        
        router->stop();
        LOG_INFO("Router stopped");
        
        public_thread->stop();
        LOG_INFO("PublicChatThreadHandler stopped");

        list_users_thread->stop();
        LOG_INFO("ListUsersThreadHandler stopped");

        join_thread->stop();
        LOG_INFO("JoinPublicChatRoomThreadHandler stopped");

        leave_thread->stop();
        LOG_INFO("LeavePublicChatRoomThreadHandler stopped");

        response_dispatcher->stop();
        LOG_INFO("Response Dispatcher stopped");
        
        server->stopServer();
        LOG_INFO("TCP Server stopped");

        LOG_INFO("Clean shutdown complete");
        
    } 
    catch(const std::exception& e){
        LOG_ERROR_STREAM("Fatal error: " << e.what());
        return 1;
    }
    
    return 0;
}