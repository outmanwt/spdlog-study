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
#include <fstream>
#include <string>

class file_helper {
public:
    file_helper() : is_open_(false) {}

    void open(const std::string& filename, bool truncate = false) {
        std::ios_base::openmode mode = std::ios_base::out | std::ios_base::app;
        if (truncate) {
            mode |= std::ios_base::trunc;
        }
        file_stream_.open(filename, mode);
        if (!file_stream_.is_open()) {
            throw std::runtime_error("无法打开文件：" + filename);
        }
        is_open_ = true;
    }

    void write(const std::string& msg) {
        if (!is_open_) {
            throw std::runtime_error("文件未打开");
        }
        file_stream_ << msg;
    }

    void flush() {
        if (!is_open_) {
            throw std::runtime_error("文件未打开");
        }
        file_stream_.flush();
    }

    void close() {
        if (is_open_) {
            file_stream_.close();
            is_open_ = false;
        }
    }

    ~file_helper() {
        close();
    }

private:
    std::ofstream file_stream_;
    bool is_open_;
};
class base_sink {
public:
    virtual ~base_sink() = default;

    virtual void log(const std::string& msg) = 0;
    virtual void flush() = 0;
};

class ansicolor_sink : public base_sink {
public:
    ansicolor_sink(FILE* file) : file_(file) {}

    void log(const std::string& msg) override {
        std::lock_guard<std::mutex> lock(mutex_);
        fmt::print(file_, "{}", msg);
    }

    void flush() override {
        std::lock_guard<std::mutex> lock(mutex_);
        fflush(file_);
    }


private:
    FILE* file_;

    std::mutex mutex_;
};

class file_sink : public base_sink {
public:
    file_sink(const std::string& filename) : filename_(filename) {
        file_helper_.open(filename, false);
    }

    void log(const std::string& msg) override {
        std::lock_guard<std::mutex> lock(mutex_);
        file_helper_.write(msg);
    }

    void flush() override {
        std::lock_guard<std::mutex> lock(mutex_);
        file_helper_.flush();
    }



private:
    std::string filename_;
    file_helper file_helper_;

    std::mutex mutex_;
};

class Logger {
public:
	enum LogLevel {
		INFO,
		WARNING,
		ERROR
	};
    Logger(){}

    virtual ~Logger() {
    }

    void add_sink(std::shared_ptr<base_sink> sink) {
        std::lock_guard<std::mutex> lock(sinks_mutex_);
        sinks_.push_back(sink);
    }

    template <typename... Args>
    void log(LogLevel level, const std::string& format, Args... args) {
        std::lock_guard<std::mutex> lock(log_mutex);
        auto log_entry = fmt::format("[{}] [{}] {}\n", currentDateTime(), toString(level), fmt::format(format, args...));
        write_to_sinks(log_entry, level);
    }
    
    void set_level(LogLevel log_level) {
        level_.store(log_level);
    }

    LogLevel level() const {
        return level_.load(std::memory_order_relaxed);
    }
	
protected:
    mutable std::mutex log_mutex;
    std::vector<std::shared_ptr<base_sink>> sinks_;
    std::mutex sinks_mutex_;
    std::atomic<LogLevel> level_{LogLevel::INFO};
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

    void write_to_sinks(const std::string& log_entry, LogLevel level) {
        std::lock_guard<std::mutex> lock(sinks_mutex_);
        for (auto& sink : sinks_) {
            if (level >= level_) {
                sink->log(log_entry);
            }
        }
    }
};

class AsyncLogger : public Logger {
public:
    AsyncLogger(size_t poolSize = 1)
        : log_pool(poolSize, 1000, async_overflow_policy::block) {}

    ~AsyncLogger() {
        shutdown();
    }

    void shutdown() {
    }

    template <typename... Args>
    void log(LogLevel level, const std::string& format, Args... args) {
        log_pool.enqueue([this, level, format, args...] {
            std::lock_guard<std::mutex> lock(log_mutex);
            auto log_entry = fmt::format("[{}] [{}] {}\n", currentDateTime(), toString(level), fmt::format(format, args...));
            write_to_sinks(log_entry, level);
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
        auto syncLogger = std::make_shared<Logger>();
        Registry::getInstance().registerLogger("sync", syncLogger);

        // 创建异步日志记录器并注册到 Registry
        auto asyncLogger = std::make_shared<AsyncLogger>(4);
        Registry::getInstance().registerLogger("async", asyncLogger);

        // 创建 sink
        auto console_sink = std::make_shared<ansicolor_sink>(stdout);
        auto _file_sink = std::make_shared<file_sink>("log.txt");

        // 将 sink 添加到同步日志记录器
        auto sync = Registry::getInstance().getLogger("sync");
        sync->add_sink(console_sink);
        sync->add_sink(_file_sink);

        // 将 sink 添加到异步日志记录器
        auto async = Registry::getInstance().getLogger("async");
        async->add_sink(console_sink);
        async->add_sink(_file_sink);

        // 从 Registry 获取同步日志记录器并记录日志
        sync->log(Logger::INFO, "这是一条同步日志。");
        sync->log(Logger::WARNING, "这是一条同步警告日志。");
        sync->log(Logger::ERROR, "这是一条同步错误日志。错误代码：{}。错误信息：{}", 404, "未找到");

        // 从 Registry 获取异步日志记录器并记录日志
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
