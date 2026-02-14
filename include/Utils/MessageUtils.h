#pragma once

#include <string>
#include <algorithm>

class MessageUtils{
    public:
        static std::string sanitize(const std::string& msg);   
        static std::string sanitizeReplace(const std::string& msg); 
        static bool isValid(const std::string& msg);
};