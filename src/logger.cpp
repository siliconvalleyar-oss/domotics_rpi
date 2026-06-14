#include "logger.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>
#include <cerrno>
#include <sstream>

std::ofstream Logger::file_;
std::mutex Logger::mutex_;
bool Logger::initialized_ = false;

void Logger::init(const std::string& logDir, const std::string& filename) {
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (initialized_) return;

        struct stat st;
        if (stat(logDir.c_str(), &st) != 0) {
            mkdir(logDir.c_str(), 0755);
        }

        std::string path = logDir + "/" + filename;
        file_.open(path, std::ios::app);
        if (!file_.is_open()) {
            std::cerr << "[LOGGER] Failed to open log file: " << path
                      << " (" << strerror(errno) << ")" << std::endl;
            return;
        }

        initialized_ = true;
    }
    info("LOGGER", "=== Log started ===");
}

void Logger::shutdown() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (initialized_) {
            initialized_ = false;
        }
    }
    info("LOGGER", "=== Log ended ===");
    if (file_.is_open()) {
        file_.close();
    }
}

void Logger::debug(const std::string& module, const std::string& msg) {
    log(LOG_DEBUG, module, msg);
}

void Logger::info(const std::string& module, const std::string& msg) {
    log(LOG_INFO, module, msg);
}

void Logger::warn(const std::string& module, const std::string& msg) {
    log(LOG_WARN, module, msg);
}

void Logger::error(const std::string& module, const std::string& msg) {
    log(LOG_ERROR, module, msg);
}

void Logger::log(LogLevel level, const std::string& module, const std::string& msg) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string line = "[" + currentTimestamp() + "] [" + levelToString(level)
                       + "] [" + module + "] " + msg;

    if (level <= LOG_WARN) {
        std::cout << line << std::endl;
    } else {
        std::cerr << line << std::endl;
    }

    if (file_.is_open()) {
        file_ << line << std::endl;
        file_.flush();
    }
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO:  return "INFO";
        case LOG_WARN:  return "WARN";
        case LOG_ERROR: return "ERROR";
        default:        return "UNKN";
    }
}

std::string Logger::currentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) % 1000;

    struct tm tm_buf;
    localtime_r(&in_time_t, &tm_buf);

    std::ostringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S")
       << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}
