#include "MessageUtils.h"

std::string MessageUtils::sanitize(const std::string& msg){
    std::string result = msg;
    
    result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
    result.erase(std::remove(result.begin(), result.end(), '\r'), result.end());
    
    result.erase(std::remove(result.begin(), result.end(), '\0'), result.end());
    
    size_t start = result.find_first_not_of(" \t");
    if(start == std::string::npos) return "";
    
    size_t end = result.find_last_not_of(" \t");
    return result.substr(start, end - start + 1);
}

std::string MessageUtils::sanitizeReplace(const std::string& msg){
    std::string result = msg;
    
    std::replace(result.begin(), result.end(), '\n', ' ');
    std::replace(result.begin(), result.end(), '\r', ' ');
    
    result.erase(std::remove(result.begin(), result.end(), '\0'), result.end());
    
    return result;
}

bool MessageUtils::isValid(const std::string& msg){
    if(msg.find('\n') != std::string::npos) return false;
    if(msg.find('\r') != std::string::npos) return false;
    if(msg.find('\0') != std::string::npos) return false;
    
    return true;
}