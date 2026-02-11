#include "TimeUtils.h"

std::string TimeUtils::getCurrentTimestamp(){
    return formatTimestamp(std::chrono::system_clock::now());
}

std::string TimeUtils::formatTimestamp(const std::chrono::system_clock::time_point& tp){
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return oss.str();
}