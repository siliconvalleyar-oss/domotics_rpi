#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>

enum LogLevel {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
};

class Logger {
public:
    static void init(const std::string& logDir = "logs", const std::string& filename = "domotics_rpi.log");
    static void shutdown();

    static void debug(const std::string& module, const std::string& msg);
    static void info(const std::string& module, const std::string& msg);
    static void warn(const std::string& module, const std::string& msg);
    static void error(const std::string& module, const std::string& msg);
    static void log(LogLevel level, const std::string& module, const std::string& msg);

private:
    static std::string levelToString(LogLevel level);
    static std::string currentTimestamp();

    static std::ofstream file_;
    static std::mutex mutex_;
    static bool initialized_;
};

#endif // LOGGER_H
