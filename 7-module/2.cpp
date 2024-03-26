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

    Logger(const std::string& logFilePath) : log_file(logFilePath, std::ios::app) {
        if (!log_file.is_open()) {
            throw std::runtime_error("无法打开日志文件！");
        }
    }

    virtual ~Logger() {
        log_file.close();
    }

    template <typename... Args>
    void log(LogLevel level, const std::string& format, Args... args) {
        std::lock_guard<std::mutex> lock(log_mutex);
        auto log_entry = fmt::format("[{}] [{}] {}\n", currentDateTime(), toString(level), fmt::format(format, args...));
        log_file << log_entry;
    }

protected:
    std::ofstream log_file;
    mutable std::mutex log_mutex;

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

class AsyncLogger : public Logger {
public:
    AsyncLogger(const std::string& logFilePath, size_t poolSize = 1)
        : Logger(logFilePath), log_pool(poolSize, 1000, async_overflow_policy::block) {}

    ~AsyncLogger() {
        shutdown();
    }

    void shutdown() {
        log_file.close();
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
    ThreadPool log_pool;
};

int main() {
    try {
        // 创建同步日志记录器
        std::shared_ptr<Logger> syncLogger = std::make_shared<Logger>("sync.log");

        // 创建异步日志记录器
        std::shared_ptr<AsyncLogger> asyncLogger = std::make_shared<AsyncLogger>("async.log", 4);

        // 使用同步日志记录器记录日志
        syncLogger->log(Logger::INFO, "这是一条同步日志。");
        syncLogger->log(Logger::WARNING, "这是一条同步警告日志。");
        syncLogger->log(Logger::ERROR, "这是一条同步错误日志。错误代码：{}。错误信息：{}", 404, "未找到");

        // 使用异步日志记录器记录日志
        asyncLogger->log(Logger::INFO, "这是一条异步日志。");
        asyncLogger->log(Logger::WARNING, "这是一条异步警告日志。");
        asyncLogger->log(Logger::ERROR, "这是一条异步错误日志。错误代码：{}。错误信息：{}", 500, "内部服务器错误");

        // 模拟程序执行一段时间
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // 关闭异步日志记录器
        asyncLogger->shutdown();

    } catch (const std::exception& e) {
        std::cerr << "日志记录器初始化失败: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
