#pragma once

#include <unordered_set>
#include <mutex>

class PublicChatRoom{
    private:
        std::unordered_set<int> participants;
        std::mutex room_mutex;
    public:
        PublicChatRoom() = default;
        ~PublicChatRoom() = default;

        static PublicChatRoom& getInstance();
        void join(int fd);
        void leave(int fd);
        std::unordered_set<int> getParticipants();
        bool isParticipant(int fd);
        size_t getParticipantsCount();
};