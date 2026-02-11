#pragma once

#include <string>
#include <chrono>
#include <sstream>
#include <iomanip>

class TimeUtils{
    public:
        static std::string getCurrentTimestamp();
        static std::string formatTimestamp(const std::chrono::system_clock::time_point& tp);
};