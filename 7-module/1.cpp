#include <fstream>
#include <iostream>
#include <string>
#include <ctime>
#include <fmt/core.h>
#include <thread>
#include <chrono>
#include <mutex>
#include "include/ThreadPool.h"

class Logger {
public:
    enum LogLevel {
        INFO,
        WARNING,
        ERROR
    };

    Logger(const std::string& logFilePath, size_t poolSize = 1) : log_file(logFilePath, std::ios::app), log_pool(poolSize, 1000, async_overflow_policy::block) {
        if (!log_file.is_open()) {
            throw std::runtime_error("无法打开日志文件！");
        }
    }

    ~Logger() {
        shutdown();
    }

    // Shutdown method to stop accepting new log messages and flush all pending logs
    void shutdown() {
        // Since the ThreadPool destructor will join all threads,
        // we just need to ensure that no new tasks are enqueued after calling shutdown.
        log_file.close(); // Close the log file after all tasks are done.
    }

    template <typename... Args>
    void log(LogLevel level, const std::string& format, Args... args) {
        log_pool.enqueue([this, level, format, args...] {
            std::lock_guard<std::mutex> lock(log_mutex);
            auto log_entry = fmt::format("[{}] [{}] {}\n", currentDateTime(), toString(level), fmt::format(format, args...));
            log_file << log_entry;
        });
    }

private:
    std::ofstream log_file;
    ThreadPool log_pool;
    mutable std::mutex log_mutex; // 保护文件写入操作


    const char* toString(LogLevel level) const {
        switch(level) {
            case INFO: return "INFO";
            case WARNING: return "WARNING";
            case ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }

    std::string currentDateTime() const {
        std::time_t now = std::time(nullptr);
        std::string dt = std::ctime(&now);
        dt.pop_back(); // 移除换行符
        return dt;
    }
};

int main() {
    try {
        Logger logger("log.txt", 10);

        logger.log(Logger::INFO, "这是一条信息级别的消息。");
        logger.log(Logger::ERROR, "错误代码：{}. 错误信息：{}", 404, "未找到");
    } catch(const std::exception& e) {
        std::cerr << "日志器初始化失败: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}
