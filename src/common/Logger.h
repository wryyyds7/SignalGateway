#pragma once
#include <string>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

enum class LogLevel { DEBUG, INFO, WARN, ERROR };
class Logger {
public:
    static LogLevel level;
    static void setLevel(LogLevel l) { level = l; }
    static std::string timestamp() {
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        std::ostringstream ss;
        ss << std::put_time(std::localtime(&t), "%H:%M:%S") << "." << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }
};
inline LogLevel Logger::level = LogLevel::INFO;
#define LOG(lvl, msg) do { if (lvl >= Logger::level) { \
    const char* _t = (lvl==LogLevel::DEBUG)?"DBG":(lvl==LogLevel::INFO)?"INF":(lvl==LogLevel::WARN)?"WRN":"ERR"; \
    std::cout << "[" << Logger::timestamp() << "][" << _t << "] " << msg << std::endl; } } while(0)
#define LOG_INFO(msg) LOG(LogLevel::INFO, msg)
#define LOG_DEBUG(msg) LOG(LogLevel::DEBUG, msg)
#define LOG_WARN(msg) LOG(LogLevel::WARN, msg)
#define LOG_ERROR(msg) LOG(LogLevel::ERROR, msg)
