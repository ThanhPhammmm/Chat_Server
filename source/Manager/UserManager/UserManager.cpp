#include "UserManager.h"

UserManager& UserManager::getInstance(){
    static UserManager instance;
    return instance;
}

void UserManager::loginUser(int fd, std::string& username, int user_id){
    std::lock_guard<std::mutex> lock(user_mutex);
    
    fd_to_username[fd] = username;
    username_to_fd[username] = fd;
    fd_to_userid[fd] = user_id;
}

void UserManager::logoutUser(int fd){
    std::lock_guard<std::mutex> lock(user_mutex);
    
    auto it = fd_to_username.find(fd);
    if(it != fd_to_username.end()){
        username_to_fd.erase(it->second);
        fd_to_username.erase(it);
    }
    fd_to_userid.erase(fd);
}

std::optional<std::string> UserManager::getUsername(int fd){
    std::lock_guard<std::mutex> lock(user_mutex);
    auto it = fd_to_username.find(fd);
    if(it != fd_to_username.end()){
        return it->second;
    }
    return std::nullopt;
}

std::optional<int> UserManager::getFd(std::string& username){
    std::lock_guard<std::mutex> lock(user_mutex);
    auto it = username_to_fd.find(username);
    if(it != username_to_fd.end()){
        return it->second;
    }
    return std::nullopt;
}

std::optional<int> UserManager::getUserId(int fd){
    std::lock_guard<std::mutex> lock(user_mutex);
    auto it = fd_to_userid.find(fd);
    if(it != fd_to_userid.end()){
        return it->second;
    }
    return std::nullopt;
}

bool UserManager::isLoggedIn(int fd){
    std::lock_guard<std::mutex> lock(user_mutex);
    return fd_to_username.find(fd) != fd_to_username.end();
}

bool UserManager::isUsernameLoggedIn(std::string& username){
    std::lock_guard<std::mutex> lock(user_mutex);
    return username_to_fd.find(username) != username_to_fd.end();
}

std::vector<std::pair<int, std::string>> UserManager::getAllLoggedInUsers(){
    std::lock_guard<std::mutex> lock(user_mutex);
    std::vector<std::pair<int, std::string>> users;
    for(const auto& pair : fd_to_username){
        users.push_back(pair);
    }
    return users;
}