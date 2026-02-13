#pragma once

#include "Connection.h"

enum class MessageType {
    INCOMING_MESSAGE,    // Client -> Server
    ACKNOWLEDGMENT,
    SHUTDOWN
};

struct IncomingMessage{
    ConnectionPtr connection;
    std::string content;
    int fd;
};

using MessagePayload = std::variant<
    IncomingMessage
>;

struct Message{
    MessageType type;
    MessagePayload payload;
};