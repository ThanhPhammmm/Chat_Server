#pragma once

#include <sqlite3.h>
#include <string>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

struct User{
    int id;
    std::string username;
    std::string password_hash;
    std::string created_at;
    std::string last_login;
};

class DataBaseManager{
    private:
        sqlite3* db;
        std::mutex db_mutex;
        std::string db_path;
        
        std::string hashPassword(const std::string& password);
        
    public:
        DataBaseManager(const std::string& db_file);
        ~DataBaseManager();
        
        static DataBaseManager& getInstance();
        
        bool initialize();
        bool registerUser(const std::string& username, const std::string& password);
        bool verifyLogin(const std::string& username, const std::string& password);
        bool usernameExists(const std::string& username);
        bool updateLastLogin(const std::string& username);
        std::optional<User> getUser(const std::string& username);
        std::vector<std::string> getAllUsernames();
};

using DatabaseManagerPtr = std::shared_ptr<DataBaseManager>;