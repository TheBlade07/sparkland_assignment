#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <memory>

namespace sparkland{

class Logger {
public:

    // Delete copy/move operations
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    // Get the singleton instance
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void log(const std::string& message, const std::string& level = "INFO") {
        std::lock_guard<std::mutex> lock(logMutex_);
        
        if (!logFile_.is_open()) {
            std::cerr << "Log file is not open!" << std::endl;
            return;
        }

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        logFile_ << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
                 << "." << std::setfill('0') << std::setw(3) << ms.count()
                 << "] [" << level << "] " << message << std::endl;
        
        // Flush to ensure data is written to disk
        logFile_.flush();
    }

    void info(const std::string& message) {
        log(message, "INFO");
    }

    void warning(const std::string& message) {
        log(message, "WARNING");
    }

    void error(const std::string& message) {
        log(message, "ERROR");
    }

    void debug(const std::string& message) {
        log(message, "DEBUG");
    }

    // Set log file (can be called only once)
    bool setLogFile(const std::string& filename) {
        std::lock_guard<std::mutex> lock(logMutex_);
        
        if (logFile_.is_open()) {
            logFile_.close();
        }
        
        logFile_.open(filename, std::ios::out | std::ios::app);
        return logFile_.is_open();
    }

    // Close the log file
    void close() {
        std::lock_guard<std::mutex> lock(logMutex_);
        if (logFile_.is_open()) {
            logFile_.close();
        }
    }

private:
    Logger() {
        logFile_.open("application.log", std::ios::out | std::ios::app);
        if (!logFile_.is_open()) {
            std::cerr << "Failed to open log file!" << std::endl;
        }
    }

    ~Logger() {
        close();
    }

    std::ofstream logFile_;
    std::mutex logMutex_;
};
}


#endif