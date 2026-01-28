#include "Logger.h"

Logger::Logger() : current_level(LogLevel::INFO){
    worker = std::thread([this](){ workerThread(); });
}

Logger::~Logger(){
    stop();
}

Logger& Logger::getInstance(){
    static Logger instance;
    return instance;
}

void Logger::stop(){
    if(running.load()){
        running.store(false);
        queue_cv.notify_all();
        
        if(worker.joinable()){
            worker.join();
        }
        
        if(log_file.is_open()){
            log_file.flush();
            log_file.close();
        }
    }
}

void Logger::flush(){
    if(!running.load()){
        return;  // Don't wait if worker already stopped
    }
    
    std::unique_lock<std::mutex> lock(queue_mutex);
    queue_cv.wait_for(lock, std::chrono::seconds(2), [this]{ 
        return log_queue.empty() || !running.load(); 
    });
}

void Logger::workerThread(){
    while(running.load()){
        std::unique_lock<std::mutex> lock(queue_mutex);
        
        queue_cv.wait(lock, [this]{ 
            return !log_queue.empty() || !running.load(); 
        });
        
        while(!log_queue.empty()){
            LogMessage msg = std::move(log_queue.front());
            log_queue.pop();
            lock.unlock();
            
            processLog(msg);
            
            lock.lock();
        }
    }
    
    std::unique_lock<std::mutex> lock(queue_mutex);
    while(!log_queue.empty()){
        LogMessage msg = std::move(log_queue.front());
        log_queue.pop();
        lock.unlock();
        
        processLog(msg);
        
        lock.lock();
    }
}

void Logger::processLog(const LogMessage& msg){
    if(msg.level < current_level){
        return;
    }
    
    std::ostringstream log_stream;
    log_stream << "[" << getCurrentTime(msg.timestamp) << "] "
               << "[" << logLevelToString(msg.level) << "] "
               << "[TID:" << msg.thread_id << "] "
               << msg.message;
    
    if(!msg.file.empty() && msg.line > 0){
        size_t pos = msg.file.find_last_of("/\\");
        std::string filename = (pos != std::string::npos) ? msg.file.substr(pos + 1) : msg.file;
        log_stream << " (" << filename << ":" << msg.line << ")";
    }
    
    std::string log_message = log_stream.str();
    
    if(console_output.load()){
        std::cout << getColorCode(msg.level) << log_message << "\033[0m" << std::endl;
    }
    
    if(file_output.load() && log_file.is_open()){
        log_file << log_message << std::endl;
        
        if(msg.level == LogLevel::ERROR){
            log_file.flush();
        }
    }
}

std::string Logger::getCurrentTime(const std::chrono::system_clock::time_point& tp){
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()) % 1000;
    
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
    current_level = level;
}

void Logger::setConsoleOutput(bool enable){
    console_output.store(enable);
}

void Logger::setFileOutput(bool enable){
    file_output.store(enable);
}

void Logger::setLogFile(const std::string& filename){
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
        file_output.store(false);
    } 
    else{
        file_output.store(true);
        log_file << "\n========== New Session: " 
                 << getCurrentTime(std::chrono::system_clock::now()) 
                 << " ==========\n";
        log_file.flush();
    }
}

void Logger::pushLog(LogLevel level, const std::string& message, const std::string& file, int line){
    if(level != current_level){
        return;
    }
    
    LogMessage msg;
    msg.level = level;
    msg.message = message;
    msg.file = file;
    msg.line = line;
    msg.timestamp = std::chrono::system_clock::now();
    msg.thread_id = std::this_thread::get_id();
    
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        log_queue.push(std::move(msg));
    }
    queue_cv.notify_one();
}

void Logger::debug(const std::string& message, const std::string& file, int line){
    pushLog(LogLevel::DEBUG, message, file, line);
}

void Logger::info(const std::string& message, const std::string& file, int line){
    pushLog(LogLevel::INFO, message, file, line);
}

void Logger::warning(const std::string& message, const std::string& file, int line){
    pushLog(LogLevel::WARNING, message, file, line);
}

void Logger::error(const std::string& message, const std::string& file, int line){
    pushLog(LogLevel::ERROR, message, file, line);
}