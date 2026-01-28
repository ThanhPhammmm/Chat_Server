#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <thread>
#include <filesystem>
#include <queue>
#include <condition_variable>
#include <atomic>

enum class LogLevel{
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

struct LogMessage{
    LogLevel level;
    std::string message;
    std::string file;
    int line;
    std::chrono::system_clock::time_point timestamp;
    std::thread::id thread_id;
};

class Logger{
public:
    static Logger& getInstance();

    void setLogLevel(LogLevel level);
    void setConsoleOutput(bool enable);
    void setFileOutput(bool enable);
    void setLogFile(const std::string& filename);

    void debug(const std::string& message,
               const std::string& file = "", 
               int line = 0);
    void info(const std::string& message,
              const std::string& file = "", 
              int line = 0);
    void warning(const std::string& message,
                 const std::string& file = "", 
                 int line = 0);
    void error(const std::string& message,
               const std::string& file = "", 
               int line = 0);

    void flush();
    void stop();

private:
    Logger();
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string getCurrentTime(const std::chrono::system_clock::time_point& tp);
    std::string logLevelToString(LogLevel level);
    std::string getColorCode(LogLevel level);
    void processLog(const LogMessage& msg);
    void workerThread();
    
    void pushLog(LogLevel level, const std::string& message,const std::string& file, int line);

    std::ofstream log_file;
    std::queue<LogMessage> log_queue;
    std::mutex queue_mutex;
    std::condition_variable queue_cv;
    std::atomic<bool> running{true};
    std::thread worker;
    
    LogLevel current_level;
    std::atomic<bool> console_output{true};
    std::atomic<bool> file_output{true};
};

#define LOG_DEBUG(msg) \
    Logger::getInstance().debug(msg, __FILE__, __LINE__)

#define LOG_INFO(msg) \
    Logger::getInstance().info(msg, __FILE__, __LINE__)

#define LOG_WARNING(msg) \
    Logger::getInstance().warning(msg, __FILE__, __LINE__)

#define LOG_ERROR(msg) \
    Logger::getInstance().error(msg, __FILE__, __LINE__)

#define LOG_DEBUG_STREAM(stream) \
    do{ std::ostringstream oss; oss << stream; \
         Logger::getInstance().debug(oss.str(), __FILE__, __LINE__); } while (0)

#define LOG_INFO_STREAM(stream) \
    do{ std::ostringstream oss; oss << stream; \
         Logger::getInstance().info(oss.str(), __FILE__, __LINE__); } while (0)

#define LOG_WARNING_STREAM(stream) \
    do{ std::ostringstream oss; oss << stream; \
         Logger::getInstance().warning(oss.str(), __FILE__, __LINE__); } while (0)

#define LOG_ERROR_STREAM(stream) \
    do{ std::ostringstream oss; oss << stream; \
         Logger::getInstance().error(oss.str(), __FILE__, __LINE__); } while (0)
