#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <vector>

class UserManager{
    private:
        std::unordered_map<int, std::string> fd_to_username;
        std::unordered_map<std::string, int> username_to_fd;
        std::unordered_map<int, int> fd_to_userid;
        std::mutex user_mutex;
        
    public:
        UserManager() = default;
        ~UserManager() = default;
        
        static UserManager& getInstance();
        
        void loginUser(int fd, std::string& username, int user_id);
        void logoutUser(int fd);
        
        std::optional<std::string> getUsername(int fd);
        std::optional<int> getFd(std::string& username);
        std::optional<int> getUserId(int fd);
        bool isLoggedIn(int fd);
        bool isUsernameLoggedIn(std::string& username);
        
        std::vector<std::pair<int, std::string>> getAllLoggedInUsers();
};