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

    const char* create_pending_messages_table = 
        "CREATE TABLE IF NOT EXISTS pending_messages ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "message_id TEXT UNIQUE NOT NULL,"
        "sender_id INTEGER NOT NULL,"
        "receiver_id INTEGER NOT NULL,"
        "status TEXT DEFAULT 'pending',"  // pending, sent, acknowledged, failed
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "last_retry_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "retry_count INTEGER DEFAULT 0,"
        "FOREIGN KEY (sender_id) REFERENCES users(id),"
        "FOREIGN KEY (receiver_id) REFERENCES users(id)"
        ");";
    
    rc = sqlite3_exec(db, create_pending_messages_table, nullptr, nullptr, &err_msg);
    
    if(rc != SQLITE_OK){
        LOG_ERROR_STREAM("SQL error creating pending_messages table: " << err_msg);
        sqlite3_free(err_msg);
        return false;
    }
    
    // Create index for faster queries
    const char* create_index = 
        "CREATE INDEX IF NOT EXISTS idx_pending_messages_status "
        "ON pending_messages(status);";
    
    rc = sqlite3_exec(db, create_index, nullptr, nullptr, &err_msg);
    
    if(rc != SQLITE_OK){
        LOG_ERROR_STREAM("SQL error creating index: " << err_msg);
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

bool DataBaseManager::addPendingMessage(const std::string& message_id, int sender_id, int receiver_id){
    std::lock_guard<std::mutex> lock(db_mutex);
    
    if(!db) return false;
    
    const char* sql = 
        "INSERT INTO pending_messages (message_id, sender_id, receiver_id, status) "
        "VALUES (?, ?, ?, 'pending');";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if(rc != SQLITE_OK){
        LOG_ERROR_STREAM("Failed to prepare add pending message: " << sqlite3_errmsg(db));
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, message_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, sender_id);
    sqlite3_bind_int(stmt, 3, receiver_id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if(rc != SQLITE_DONE){
        LOG_ERROR_STREAM("Failed to add pending message: " << sqlite3_errmsg(db));
        return false;
    }
    
    LOG_DEBUG_STREAM("Added pending message: " << message_id);
    return true;
}

bool DataBaseManager::updateMessageStatus(const std::string& message_id, const std::string& status){
    std::lock_guard<std::mutex> lock(db_mutex);
    
    if(!db) return false;
    
    const char* sql = 
        "UPDATE pending_messages SET status = ?, last_retry_at = CURRENT_TIMESTAMP "
        "WHERE message_id = ?;";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if(rc != SQLITE_OK){
        LOG_ERROR_STREAM("Failed to prepare update status: " << sqlite3_errmsg(db));
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, message_id.c_str(), -1, SQLITE_TRANSIENT);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if(rc != SQLITE_DONE){
        LOG_ERROR_STREAM("Failed to update message status: " << sqlite3_errmsg(db));
        return false;
    }
    
    LOG_DEBUG_STREAM("Updated message " << message_id << " status to: " << status);
    return true;
}

bool DataBaseManager::deletePendingMessage(const std::string& message_id){
    std::lock_guard<std::mutex> lock(db_mutex);
    
    if(!db) return false;
    
    const char* sql = "DELETE FROM pending_messages WHERE message_id = ?;";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if(rc != SQLITE_OK){
        LOG_ERROR_STREAM("Failed to prepare delete: " << sqlite3_errmsg(db));
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, message_id.c_str(), -1, SQLITE_TRANSIENT);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if(rc != SQLITE_DONE){
        LOG_ERROR_STREAM("Failed to delete pending message: " << sqlite3_errmsg(db));
        return false;
    }
    
    LOG_DEBUG_STREAM("Deleted pending message: " << message_id);
    return true;
}

bool DataBaseManager::incrementRetryCount(const std::string& message_id){
    std::lock_guard<std::mutex> lock(db_mutex);
    
    if(!db) return false;
    
    const char* sql = 
        "UPDATE pending_messages SET retry_count = retry_count + 1, "
        "last_retry_at = CURRENT_TIMESTAMP WHERE message_id = ?;";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if(rc != SQLITE_OK){
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, message_id.c_str(), -1, SQLITE_TRANSIENT);
    
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

std::vector<PendingMessageRecord> DataBaseManager::getPendingMessages(){
    std::lock_guard<std::mutex> lock(db_mutex);
    
    std::vector<PendingMessageRecord> records;
    if(!db) return records;
    
    const char* sql = 
        "SELECT id, message_id, sender_id, receiver_id, status, "
        "created_at, last_retry_at, retry_count "
        "FROM pending_messages WHERE status = 'pending' OR status = 'sent' "
        "ORDER BY created_at ASC;";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if(rc != SQLITE_OK){
        LOG_ERROR_STREAM("Failed to prepare get pending messages: " << sqlite3_errmsg(db));
        return records;
    }
    
    while(sqlite3_step(stmt) == SQLITE_ROW){
        PendingMessageRecord rec;
        rec.id = sqlite3_column_int(stmt, 0);
        rec.message_id = (const char*)sqlite3_column_text(stmt, 1);
        rec.sender_id = sqlite3_column_int(stmt, 2);
        rec.receiver_id = sqlite3_column_int(stmt, 3);
        rec.status = (const char*)sqlite3_column_text(stmt, 4);
        rec.created_at = (const char*)sqlite3_column_text(stmt, 5);
        rec.last_retry_at = (const char*)sqlite3_column_text(stmt, 6);
        rec.retry_count = sqlite3_column_int(stmt, 7);
        
        records.push_back(rec);
    }
    
    sqlite3_finalize(stmt);
    return records;
}