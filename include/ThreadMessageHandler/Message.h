#pragma once

#include "Connection.h"

enum class MessageType {
    INCOMING_MESSAGE,    // Client → Server
    // OUTGOING_RESPONSE,   // Server → Client
    // BROADCAST,           // Server → All Clients
    // CLIENT_DISCONNECTED,
    SHUTDOWN
};

struct IncomingMessage{
    ConnectionPtr connection;
    std::string content;
    int fd;
};

struct OutgoingResponse{
    ConnectionPtr connection;
    std::string content;
};

// struct BroadcastMessage{
//     std::string content;
//     int exclude_fd;
// };

// struct ClientDisconnected{
//     int fd;
// };

using MessagePayload = std::variant<
    IncomingMessage,
    OutgoingResponse
    // BroadcastMessage,
    // ClientDisconnected
>;

struct Message{
    MessageType type;
    MessagePayload payload;
};