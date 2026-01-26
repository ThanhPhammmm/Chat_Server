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
    bool is_public_room;
    bool is_private_room;
    bool is_private;
    bool is_error;
    int exclude_fd;
    bool is_list_users;
};

using HandlerRequestPtr = std::shared_ptr<HandlerRequest>;
using HandlerResponsePtr = std::shared_ptr<HandlerResponse>;
