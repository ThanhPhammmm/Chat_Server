#include "PublicChatRoom.h"

PublicChatRoom& PublicChatRoom::getInstance(){
    static PublicChatRoom instance;
    return instance;
}

void PublicChatRoom::join(int fd){
    std::lock_guard<std::mutex> lock(room_mutex);
    participants.insert(fd);
}

void PublicChatRoom::leave(int fd){
    std::lock_guard<std::mutex> lock(room_mutex);
    participants.erase(fd);
}

std::unordered_set<int> PublicChatRoom::getParticipants(){
    std::lock_guard<std::mutex> lock(room_mutex);
    return participants;
}

bool PublicChatRoom::isParticipant(int fd){
    std::lock_guard<std::mutex> lock(room_mutex);
    return participants.find(fd) != participants.end();
}

size_t PublicChatRoom::getParticipantsCount(){
    std::lock_guard<std::mutex> lock(room_mutex);
    return participants.size();
}

