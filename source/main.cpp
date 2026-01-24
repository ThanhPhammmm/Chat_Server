#include "TCPServer.h"

EpollPtr g_epoll = nullptr;
std::shared_ptr<MessageQueue<Message>> g_incoming = nullptr;

void signalHandler(int signum){
    std::cout << "\n[INFO] Received signal " << signum << ", shutting down...\n";
    
    if(g_epoll) {
        g_epoll->stop();
    }
    
    if(g_incoming) {
        Message msg;
        msg.type = MessageType::SHUTDOWN;
        g_incoming->push(std::move(msg));
    }
}

int main(){
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler); 
    signal(SIGPIPE, SIG_IGN);

    try{
        std::cout << "╔════════════════════════════════════════════╗\n";
        std::cout << "║   Multi-Threaded Chat Server  ║\n";
        std::cout << "╚════════════════════════════════════════════╝\n";
        std::cout << "Main Thread: " << std::this_thread::get_id() << "\n\n";

        // 1. CREATE CORE COMPONENTS
        auto thread_pool = std::make_shared<ThreadPool>(4);
        auto epoll_instance = std::make_shared<EpollInstance>();
        g_epoll = epoll_instance;
        
        // 2. CREATE MESSAGE QUEUES
        // Epoll → Router
        auto incoming_queue = std::make_shared<MessageQueue<Message>>();
        g_incoming = incoming_queue;
        
        // Router → Handlers
        auto to_public_queue = std::make_shared<MessageQueue<HandlerRequestPtr>>();
        
        // Handlers → Response Dispatcher
        auto response_queue = std::make_shared<MessageQueue<HandlerResponsePtr>>();
        
        // 3. CREATE HANDLERS
        auto public_handler = std::make_shared<PublicChatHandler>();
        
        // 4. CREATE HANDLER THREADS
        auto public_thread = std::make_shared<PublicChatHandlerThread>(public_handler, to_public_queue, response_queue);
        
        // 5. CREATE ROUTER THREAD        
        auto router = std::make_shared<ChatControllerThread>(incoming_queue);
        router->registerHandlerQueue(CommandType::PUBLIC_CHAT, to_public_queue);
        
        // 6. CREATE RESPONSE DISPATCHER
        auto response_dispatcher = std::make_shared<Responser>(response_queue, thread_pool, epoll_instance);
        
        // 7. CREATE TCP SERVER        
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(8080);
        
        auto server = std::make_shared<TCPServer>(
            addr, epoll_instance, thread_pool, incoming_queue
        );
        
        server->startServer();
        
        // 8. START ALL THREADS        
        std::cout << "Starting threads...\n\n";
        
        public_thread->start();
        std::cout << "✓ PublicChatHandler Thread started\n";
        
        // Start router
        router->start();
        std::cout << "✓ Router Thread started\n";
        
        // Start dispatcher
        response_dispatcher->start();
        std::cout << "✓ Response Dispatcher started\n";
        
        // Start epoll (last, so everything is ready)
        EpollThread epoll_thread(epoll_instance);
        epoll_thread.start();
        std::cout << "✓ Epoll Thread started\n";
        
        std::cout << "\n╔════════════════════════════════════╗\n";
        std::cout << "║   Server Ready on port 8080        ║\n";
        std::cout << "║   Commands:                        ║\n";
        std::cout << "║   /login <user> <pass>             ║\n";
        std::cout << "║   <message> - public chat          ║\n";
        std::cout << "║   Press Ctrl+C to stop             ║\n";
        std::cout << "╚════════════════════════════════════╝\n\n";
        
        // 9. MAIN THREAD: MONITORING        
        while(!g_epoll->isStopped()) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            
            std::cout << "[STATS] Incoming:" << incoming_queue->size()
                      << " ToPublic:" << to_public_queue->size()
                      << " Responses:" << response_queue->size() << "\n";
        }
        
        // 10. GRACEFUL SHUTDOWN        
        std::cout << "\n[INFO] Shutting down threads...\n";
        
        epoll_thread.stop();
        std::cout << "✓ Epoll stopped\n";
        
        router->stop();
        std::cout << "✓ Router stopped\n";
        
        public_thread->stop();
        std::cout << "✓ PublicChatHandler stopped\n";

        response_dispatcher->stop();
        std::cout << "✓ Response Dispatcher stopped\n";
        
        std::cout << "\n[INFO] Clean shutdown complete\n";
        
    } 
    catch(const std::exception& e){
        std::cerr << "[ERROR] " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}