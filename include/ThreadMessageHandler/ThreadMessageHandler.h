#pragma once

#include <Connection.h>
#include <Command.h>

// Router -> Handler
struct HandlerRequest{
    ConnectionPtr connection;
    CommandPtr command;
    int fd;
    int request_id;
};

// Handler -> ThreadPool
struct HandlerResponse{
    ConnectionPtr connection;
    std::string response_message;
    int fd;
    int request_id;
    bool is_broadcast;
    int exclude_fd;
};

using HandlerRequestPtr = std::shared_ptr<HandlerRequest>;
using HandlerResponsePtr = std::shared_ptr<HandlerResponse>;
