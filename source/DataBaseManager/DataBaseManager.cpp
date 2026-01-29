#include "DataBaseManager.h"
#include "Logger.h"
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>

DataBaseManager::DataBaseManager(const std::string& db_file) : db(nullptr), db_path(db_file) {}

DataBaseManager::~DataBaseManager(){
    if(db){
        sqlite3_close(db);
        db = nullptr;
    }
}

DataBaseManager& DataBaseManager::getInstance(){
    static DataBaseManager instance("../DataBase/chat_server.db");
    return instance;
}

std::string DataBaseManager::hashPassword(const std::string& password){
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)password.c_str(), password.length(), hash);
    
    std::ostringstream oss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++){
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return oss.str();
}

bool DataBaseManager::initialize(){
    std::lock_guard<std::mutex> lock(db_mutex);
    
    std::filesystem::path p(db_path);
    std::filesystem::create_directories(p.parent_path());

    int rc = sqlite3_open(db_path.c_str(), &db);
    if(rc != SQLITE_OK){
        LOG_ERROR_STREAM("Cannot open database: " << sqlite3_errmsg(db));
        sqlite3_close(db);
        db = nullptr;
        return false;    
    }
    
    const char* create_table_sql = 
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "username TEXT UNIQUE NOT NULL,"
        "password_hash TEXT NOT NULL,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "last_login DATETIME"
        ");";
    
    char* err_msg = nullptr;
    rc = sqlite3_exec(db, create_table_sql, nullptr, nullptr, &err_msg);
    
    if(rc != SQLITE_OK){
        LOG_ERROR_STREAM("SQL error: " << err_msg);
        sqlite3_free(err_msg);
        return false;
    }
    LOG_DEBUG("Database initialized successfully");
    return true;
}

bool DataBaseManager::registerUser(const std::string& username, const std::string& password){
    std::lock_guard<std::mutex> lock(db_mutex);
    
    if(!db) return false;
    
    std::string pwd_hash = hashPassword(password);
    
    const char* sql = "INSERT INTO users (username, password_hash) VALUES (?, ?);";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if(rc != SQLITE_OK){
        LOG_ERROR_STREAM("Failed to prepare statement: " << sqlite3_errmsg(db));
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, pwd_hash.c_str(), -1, SQLITE_TRANSIENT);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if(rc != SQLITE_DONE){
        LOG_ERROR_STREAM("Failed to register user: " << sqlite3_errmsg(db));
        return false;
    }
    
    LOG_INFO_STREAM("User registered: " << username);
    return true;
}

bool DataBaseManager::verifyLogin(const std::string& username, const std::string& password){
    std::lock_guard<std::mutex> lock(db_mutex);
    
    if(!db) return false;
    
    std::string pwd_hash = hashPassword(password);
    
    const char* sql = "SELECT password_hash FROM users WHERE username = ?;";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if(rc != SQLITE_OK){
        LOG_ERROR_STREAM("Failed to prepare statement: " << sqlite3_errmsg(db));
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    
    rc = sqlite3_step(stmt);
    
    bool verified = false;
    if(rc == SQLITE_ROW){
        const char* stored_hash = (const char*)sqlite3_column_text(stmt, 0);
        verified = (pwd_hash == stored_hash);
    }
    
    sqlite3_finalize(stmt);
    return verified;
}

bool DataBaseManager::usernameExists(const std::string& username){
    std::lock_guard<std::mutex> lock(db_mutex);
    
    if(!db) return false;
    
    const char* sql = "SELECT COUNT(*) FROM users WHERE username = ?;";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if(rc != SQLITE_OK){
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    
    rc = sqlite3_step(stmt);
    bool exists = false;
    
    if(rc == SQLITE_ROW){
        exists = sqlite3_column_int(stmt, 0) > 0;
    }
    
    sqlite3_finalize(stmt);
    return exists;
}

bool DataBaseManager::updateLastLogin(const std::string& username){
    std::lock_guard<std::mutex> lock(db_mutex);
    
    if(!db) return false;
    
    const char* sql = "UPDATE users SET last_login = CURRENT_TIMESTAMP WHERE username = ?;";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if(rc != SQLITE_OK){
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

std::optional<User> DataBaseManager::getUser(const std::string& username){
    std::lock_guard<std::mutex> lock(db_mutex);
    
    if(!db) return std::nullopt;
    
    const char* sql = "SELECT id, username, password_hash, created_at, last_login FROM users WHERE username = ?;";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if(rc != SQLITE_OK){
        return std::nullopt;
    }
    
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    
    rc = sqlite3_step(stmt);
    
    std::optional<User> user;
    if(rc == SQLITE_ROW){
        User u;
        u.id = sqlite3_column_int(stmt, 0);
        u.username = (const char*)sqlite3_column_text(stmt, 1);
        u.password_hash = (const char*)sqlite3_column_text(stmt, 2);
        u.created_at = (const char*)sqlite3_column_text(stmt, 3);
        
        const char* last_login = (const char*)sqlite3_column_text(stmt, 4);
        u.last_login = last_login ? last_login : "";
        
        user = u;
    }
    
    sqlite3_finalize(stmt);
    return user;
}

std::vector<std::string> DataBaseManager::getAllUsernames(){
    std::lock_guard<std::mutex> lock(db_mutex);
    
    std::vector<std::string> usernames;
    if(!db) return usernames;
    
    const char* sql = "SELECT username FROM users ORDER BY username;";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if(rc != SQLITE_OK){
        return usernames;
    }
    
    while(sqlite3_step(stmt) == SQLITE_ROW){
        const char* username = (const char*)sqlite3_column_text(stmt, 0);
        usernames.push_back(username);
    }
    
    sqlite3_finalize(stmt);
    return usernames;
}