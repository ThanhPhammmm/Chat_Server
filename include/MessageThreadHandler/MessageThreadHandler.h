#pragma once

#include <Connection.h>
#include <Command.h>

// Router -> Handler
struct HandlerRequest{
    ConnectionPtr connection;
    CommandPtr command;
    int fd;
    int request_id;
    int user_desntination;
};

enum class ResponseDestination{
    DIRECT_TO_CLIENT,               // Send only to requesting client
    BROADCAST_PUBLIC_CHAT_ROOM,     // Broadcast to all in public room
    ERROR_TO_CLIENT,                // Error message to client
    BACK_TO_CLIENT,                 // Send response message to client
};

// Handler -> ThreadPool
struct HandlerResponse{
    ConnectionPtr connection;
    std::string response_message;
    int fd;
    int request_id;
    ResponseDestination destination;
    int exclude_fd;  // For broadcasts, -1 means include all
    int user_destination;
    
    HandlerResponse()
        : fd(-1), 
          request_id(-1), 
          destination(ResponseDestination::DIRECT_TO_CLIENT),
          exclude_fd(-1) {}
};

using HandlerRequestPtr = std::shared_ptr<HandlerRequest>;
using HandlerResponsePtr = std::shared_ptr<HandlerResponse>;
