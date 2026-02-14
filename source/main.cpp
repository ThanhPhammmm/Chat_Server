#include "TCPServer.h"

// Configuration constants
namespace Config {
    constexpr int QUEUE_POP_TIMEOUT_MS = 100;
    constexpr int MONITOR_INTERVAL_SEC = 30;
    constexpr size_t QUEUE_WARNING_THRESHOLD = 50;
    constexpr uint16_t SERVER_PORT = 8080;
}

std::atomic<bool> g_shutdown_requested{false};

void signalHandler(int signum){
    (void)signum;
    g_shutdown_requested.store(true, std::memory_order_release);
}

int main(){
    struct sigaction sa;
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    signal(SIGPIPE, SIG_IGN);

    // Initialize Logger
    auto& logger = Logger::getInstance();
    logger.setLogLevel(LogLevel::DEBUG);
    logger.setConsoleOutput(true);
    logger.setFileOutput(true);
    logger.setLogFile("../Record/chat_server.log");
    
    LOG_INFO("╔════════════════════════════════════════════╗");
    LOG_INFO("║          Multi-Threaded Chat Server        ║");
    LOG_INFO("╚════════════════════════════════════════════╝");
    LOG_INFO_STREAM("Main Thread: " << std::this_thread::get_id());

    try{
        // 0. START DATABASE THREAD
        LOG_DEBUG("Starting database thread...");
        auto db_thread = std::make_shared<DataBaseThread>();
        db_thread->start();
        LOG_DEBUG("Database thread started");
        
        // 0. INITIALIZE ACK MANAGER WITH DATABASE
        LOG_DEBUG("Initializing ACK manager with database...");
        auto& ackMgr = MessageAckManager::getInstance();
        ackMgr.setDatabaseThread(db_thread);
        
        // 0. Load any pending messages from previous session
        //ackMgr.loadPendingMessagesFromDB();
        LOG_DEBUG("ACK manager initialized");
        
        // 0. START ACK MANAGER THREAD
        LOG_DEBUG("Starting ACK manager thread...");
        auto ackMgrThread = std::make_shared<MessageAckThreadHandler>();
        ackMgrThread->start();
        LOG_DEBUG("ACK manager thread started");

        // 1. CREATE CORE COMPONENTS
        LOG_DEBUG("Creating core components...");
        
        auto epoll_instance = std::make_shared<EpollInstance>();
        LOG_DEBUG("EpollInstance created");
        
        // 2. CREATE MESSAGE QUEUES
        LOG_DEBUG("Creating message queues...");
        auto to_incoming_queue = std::make_shared<MessageQueue<Message>>();
        auto to_register_queue = std::make_shared<MessageQueue<HandlerRequestPtr>>();
        auto to_login_queue = std::make_shared<MessageQueue<HandlerRequestPtr>>();
        auto to_logout_queue = std::make_shared<MessageQueue<HandlerRequestPtr>>();
        auto to_public_chat_room_queue = std::make_shared<MessageQueue<HandlerRequestPtr>>();
        auto to_list_users_queue = std::make_shared<MessageQueue<HandlerRequestPtr>>();
        auto to_join_public_chat_room_queue = std::make_shared<MessageQueue<HandlerRequestPtr>>();
        auto to_leave_public_chat_room_queue = std::make_shared<MessageQueue<HandlerRequestPtr>>();
        auto to_private_chat_queue = std::make_shared<MessageQueue<HandlerRequestPtr>>();
        auto to_response_queue = std::make_shared<MessageQueue<HandlerResponsePtr>>();
        LOG_DEBUG("Message queues created");
        
        // 3. CREATE HANDLERS
        LOG_DEBUG("Creating Handlers...");
        auto register_handler = std::make_shared<RegisterAccountHandler>(db_thread);
        auto login_handler = std::make_shared<LoginChatHandler>(db_thread);
        auto logout_handler = std::make_shared<LogoutChatHandler>();
        auto public_chat_room_handler = std::make_shared<PublicChatHandler>();        
        auto list_users_handler = std::make_shared<ListUsersHandler>();
        auto join_public_chat_room_handler = std::make_shared<JoinPublicChatHandler>();
        auto leave_public_chat_room_handler = std::make_shared<LeavePublicChatHandler>();
        auto private_chat_handler = std::make_shared<PrivateChatHandler>();
        LOG_DEBUG("Handlers created");

        // 4. CREATE HANDLER THREADS
        LOG_DEBUG("Creating Threads Handlers...");
        auto register_thread = std::make_shared<RegisterAccountThreadHandler>(register_handler, to_register_queue, to_response_queue);
        auto login_thread = std::make_shared<LoginChatThreadHandler>(login_handler, to_login_queue, to_response_queue);
        auto logout_thread = std::make_shared<LogoutChatThreadHandler>(logout_handler, to_logout_queue, to_response_queue);
        auto public_chat_room_thread = std::make_shared<PublicChatThreadHandler>(public_chat_room_handler, to_public_chat_room_queue, to_response_queue);
        auto list_users_thread = std::make_shared<ListUsersThreadHandler>(list_users_handler, to_list_users_queue, to_response_queue, epoll_instance);
        auto join_public_chat_room_thread = std::make_shared<JoinPublicChatThreadHandler>(join_public_chat_room_handler, to_join_public_chat_room_queue, to_response_queue);
        auto leave_public_chat_room_thread = std::make_shared<LeavePublicChatThreadHandler>(leave_public_chat_room_handler, to_leave_public_chat_room_queue, to_response_queue);
        auto private_chat_thread = std::make_shared<PrivateChatThreadHandler>(private_chat_handler, to_private_chat_queue, to_response_queue, epoll_instance);
        LOG_DEBUG("Threads Handlers created");

        LOG_DEBUG("Creating ChatControllerThread...");
        auto router = std::make_shared<ChatControllerThread>(to_incoming_queue, to_response_queue, epoll_instance);
        router->registerHandlerQueue(CommandType::REGISTER, to_register_queue);
        router->registerHandlerQueue(CommandType::LOGIN, to_login_queue);
        router->registerHandlerQueue(CommandType::LOGOUT, to_logout_queue);
        router->registerHandlerQueue(CommandType::PUBLIC_CHAT, to_public_chat_room_queue);
        router->registerHandlerQueue(CommandType::LIST_ONLINE_USERS, to_list_users_queue);
        router->registerHandlerQueue(CommandType::JOIN_PUBLIC_CHAT_ROOM, to_join_public_chat_room_queue);
        router->registerHandlerQueue(CommandType::LEAVE_PUBLIC_CHAT_ROOM, to_leave_public_chat_room_queue);
        router->registerHandlerQueue(CommandType::LIST_USERS_IN_PUBLIC_CHAT_ROOM, to_public_chat_room_queue);
        router->registerHandlerQueue(CommandType::PRIVATE_CHAT, to_private_chat_queue);
        LOG_DEBUG("ChatControllerThread created and configured");
        
        // 6. CREATE RESPONSE DISPATCHER
        LOG_DEBUG("Creating response dispatcher...");
        auto response_dispatcher = std::make_shared<Responser>(to_response_queue, epoll_instance);
        LOG_DEBUG("Responser created");
        
        // 7. CREATE TCP SERVER
        LOG_DEBUG("Creating TCP server...");
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(Config::SERVER_PORT);
        
        auto server = std::make_shared<TCPServer>(addr, epoll_instance, to_incoming_queue);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        server->startServer();
        LOG_DEBUG("TCP Server started successfully");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 8. START ALL THREADS
        LOG_DEBUG("Starting worker threads...");
        register_thread->start();
        login_thread->start();
        logout_thread->start();
        public_chat_room_thread->start();
        list_users_thread->start();
        join_public_chat_room_thread->start();        
        leave_public_chat_room_thread->start();
        private_chat_thread->start();
        router->start();        
        response_dispatcher->start();
        EpollThread epoll_thread(epoll_instance);
        epoll_thread.start();
        LOG_DEBUG("Worker threads started");
        
        LOG_INFO("╔════════════════════════════════════╗");
        LOG_INFO_STREAM("║   Server Ready on port " << Config::SERVER_PORT << "        ║");
        LOG_INFO("║   Press Ctrl+C to stop             ║");
        LOG_INFO("╚════════════════════════════════════╝");
        
        // 9. MAIN THREAD: MONITORING
        int monitor_count = 0;
        while(!epoll_instance->isStopped() && !g_shutdown_requested.load()){
            std::this_thread::sleep_for(std::chrono::seconds(Config::MONITOR_INTERVAL_SEC));
            
            size_t in_size = to_incoming_queue->size();
            size_t pub_size = to_join_public_chat_room_queue->size();
            size_t resp_size = to_response_queue->size();
            
            LOG_DEBUG_STREAM("[STATS #" << ++monitor_count << "] "
                           << "Incoming:" << in_size << " "
                           << "Public:" << pub_size << " "
                           << "Response:" << resp_size);
            
            // Warning if queues are getting full
            if(in_size > Config::QUEUE_WARNING_THRESHOLD || 
               pub_size > Config::QUEUE_WARNING_THRESHOLD || 
               resp_size > Config::QUEUE_WARNING_THRESHOLD){
                LOG_WARNING_STREAM("High queue usage detected - "
                                 << "In:" << in_size << " "
                                 << "Pub:" << pub_size << " "
                                 << "Resp:" << resp_size);
            }
        }
        
        // 10. GRACEFUL SHUTDOWN
        LOG_DEBUG("Stopping server...");
        LOG_INFO("Initiating graceful shutdown...");
        
        server->stopServer();
        LOG_DEBUG("Server stopped");

        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); 

        epoll_thread.stop();
        LOG_DEBUG("Epoll stopped");

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        router->stop();
        LOG_DEBUG("Router stopped");

        register_thread->stop();
        LOG_DEBUG("RegisterThreadHandler stopped");

        login_thread->stop();
        LOG_DEBUG("LoginThreadHandler stopped");
        
        logout_thread->stop();
        LOG_DEBUG("LogoutThreadHandler stopped");

        public_chat_room_thread->stop();
        LOG_DEBUG("PublicChatThreadHandler stopped");

        list_users_thread->stop();
        LOG_DEBUG("ListUsersThreadHandler stopped");

        join_public_chat_room_thread->stop();
        LOG_DEBUG("JoinPublicChatRoomThreadHandler stopped");

        leave_public_chat_room_thread->stop();
        LOG_DEBUG("LeavePublicChatRoomThreadHandler stopped");

        private_chat_thread->stop();
        LOG_DEBUG("PrivateChatThreadHandler stopped");

        response_dispatcher->stop();
        LOG_DEBUG("Response Dispatcher stopped");

        ackMgrThread->stop();
        LOG_DEBUG("ACK manager thread stopped");

        db_thread->stop();
        LOG_DEBUG("Database thread stopped");

        LOG_INFO("Server stopped");
        
        // Flush remaining logs
        logger.flush();
        logger.stop();

    } 
    catch(const std::exception& e){
        LOG_ERROR_STREAM("Fatal error: " << e.what());
        return 1;
    }
    
    return 0;
}