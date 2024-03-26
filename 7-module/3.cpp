#include <fstream>
#include <iostream>
#include <string>
#include <ctime>
#include <fmt/core.h>
#include <thread>
#include <chrono>
#include <mutex>
#include <unordered_map>
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
class Registry {
public:
    static Registry& getInstance() {
        static Registry instance;
        return instance;
    }

    void registerLogger(const std::string& name, std::shared_ptr<Logger> logger) {
        std::lock_guard<std::mutex> lock(mutex_);
        loggers_[name] = logger;
    }

    std::shared_ptr<Logger> getLogger(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = loggers_.find(name);
        if (it != loggers_.end()) {
            return it->second;
        }
        return nullptr;
    }

private:
    Registry() = default;
    ~Registry() = default;
    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;

    std::unordered_map<std::string, std::shared_ptr<Logger>> loggers_;
    std::mutex mutex_;
};

int main() {
    try {
        // 创建同步日志记录器并注册到 Registry
        auto syncLogger = std::make_shared<Logger>("sync.log");
        Registry::getInstance().registerLogger("sync", syncLogger);

        // 创建异步日志记录器并注册到 Registry
        auto asyncLogger = std::make_shared<AsyncLogger>("async.log", 4);
        Registry::getInstance().registerLogger("async", asyncLogger);

        // 从 Registry 获取同步日志记录器并记录日志
        auto sync = Registry::getInstance().getLogger("sync");
        sync->log(Logger::INFO, "这是一条同步日志。");
        sync->log(Logger::WARNING, "这是一条同步警告日志。");
        sync->log(Logger::ERROR, "这是一条同步错误日志。错误代码：{}。错误信息：{}", 404, "未找到");

        // 从 Registry 获取异步日志记录器并记录日志
        auto async = Registry::getInstance().getLogger("async");
        async->log(Logger::INFO, "这是一条异步日志。");
        async->log(Logger::WARNING, "这是一条异步警告日志。");
        async->log(Logger::ERROR, "这是一条异步错误日志。错误代码：{}。错误信息：{}", 500, "内部服务器错误");

        // 模拟程序执行一段时间
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // 从 Registry 获取异步日志记录器并关闭
        auto asyncToShutdown = std::dynamic_pointer_cast<AsyncLogger>(Registry::getInstance().getLogger("async"));
        asyncToShutdown->shutdown();

    } catch (const std::exception& e) {
        std::cerr << "日志记录器初始化失败: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
