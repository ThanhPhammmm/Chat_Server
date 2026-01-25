#include "Logger.h"

Logger::Logger() 
    : current_level(LogLevel::INFO),
      console_output(true),
      file_output(false) {}

Logger::~Logger(){
    if(log_file.is_open()){
        log_file.close();
    }
}

Logger& Logger::getInstance(){
    static Logger instance;
    return instance;
}

std::string Logger::getCurrentTime(){
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return oss.str();
}

std::string Logger::logLevelToString(LogLevel level){
    switch(level){
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO ";
        case LogLevel::WARNING: return "WARN ";
        case LogLevel::ERROR:   return "ERROR";
        default:                return "UNKNW";
    }
}

std::string Logger::getColorCode(LogLevel level){
    switch(level){
        case LogLevel::DEBUG:   return "\033[36m";  // Cyan
        case LogLevel::INFO:    return "\033[32m";  // Green
        case LogLevel::WARNING: return "\033[33m";  // Yellow
        case LogLevel::ERROR:   return "\033[31m";  // Red
        default:                return "\033[0m";   // Reset
    }
}

void Logger::setLogLevel(LogLevel level){
    std::lock_guard<std::mutex> lock(log_mutex);
    current_level = level;
}

void Logger::setConsoleOutput(bool enable){
    std::lock_guard<std::mutex> lock(log_mutex);
    console_output = enable;
}

void Logger::setFileOutput(bool enable){
    std::lock_guard<std::mutex> lock(log_mutex);
    file_output = enable;
}

void Logger::setLogFile(const std::string& filename){
    std::lock_guard<std::mutex> lock(log_mutex);

    std::filesystem::path path(filename);
    if(!path.parent_path().empty()){
        std::filesystem::create_directories(path.parent_path());
    }

    if(log_file.is_open()){
        log_file.flush();
        log_file.close();
    }

    log_file.open(filename, std::ios::app);
    if(!log_file.is_open()){
        std::cerr << "Failed to open log file: " << filename << std::endl;
        file_output = false;
    } 
    else{
        file_output = true;
        log_file << "\n========== New Session: " << getCurrentTime() << " ==========\n";
        log_file.flush();
    }
}

void Logger::writeLog(LogLevel level, const std::string& message, 
                      const std::string& file, int line){
    if(level < current_level){
        return;
    }
    
    std::lock_guard<std::mutex> lock(log_mutex);
    
    std::ostringstream log_stream;
    
    log_stream << "[" << getCurrentTime() << "] "
               << "[" << logLevelToString(level) << "] "
               << "[TID:" << std::this_thread::get_id() << "] "
               << message;
    
    if(!file.empty() && line > 0){
        size_t pos = file.find_last_of("/\\");
        std::string filename = (pos != std::string::npos) ? file.substr(pos + 1) : file;
        log_stream << " (" << filename << ":" << line << ")";
    }
    
    std::string log_message = log_stream.str();
    
    if(console_output){
        std::cout << getColorCode(level) << log_message << "\033[0m" << std::endl;
    }
    
    if(file_output && log_file.is_open()){
        log_file << log_message << std::endl;
        log_file.flush();
    }
}

void Logger::debug(const std::string& message, const std::string& file, int line){
    writeLog(LogLevel::DEBUG, message, file, line);
}

void Logger::info(const std::string& message, const std::string& file, int line){
    writeLog(LogLevel::INFO, message, file, line);
}

void Logger::warning(const std::string& message, const std::string& file, int line){
    writeLog(LogLevel::WARNING, message, file, line);
}

void Logger::error(const std::string& message, const std::string& file, int line){
    writeLog(LogLevel::ERROR, message, file, line);
}