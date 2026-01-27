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

enum class LogLevel{
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class Logger{
public:
    static Logger& getInstance();

    void setLogLevel(LogLevel level);
    void setConsoleOutput(bool enable);
    void setFileOutput(bool enable);
    void setLogFile(const std::string& filename);

    void debug(const std::string& message, const std::string& file = "", int line = 0);
    void info(const std::string& message, const std::string& file = "", int line = 0);
    void warning(const std::string& message, const std::string& file = "", int line = 0);
    void error(const std::string& message, const std::string& file = "", int line = 0);

private:
    Logger();
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string getCurrentTime();
    std::string logLevelToString(LogLevel level);
    std::string getColorCode(LogLevel level);
    void writeLog(LogLevel level, const std::string& message, const std::string& file, int line);

    std::ofstream log_file;
    std::mutex log_mutex;
    LogLevel current_level;
    bool console_output;
    bool file_output;
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
