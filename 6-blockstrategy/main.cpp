#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <fmt/core.h> // 包含 fmt 库的核心功能
#include "include/ThreadPool.h" // 包含 ThreadPool 类

static std::ofstream log_file("log.txt", std::ios::app); // 静态日志文件对象
static ThreadPool log_pool(1, 1, async_overflow_policy::discard_new); // 创建一个拥有 1 个工作线程的线程池用于日志

enum LogLevel {
    INFO,
    WARNING,
    ERROR
};

// 日志级别到字符串的转换
const char* toString(LogLevel level) {
    switch(level) {
        case INFO: return "INFO";
        case WARNING: return "WARNING";
        case ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

// 获取格式化的当前时间字符串
std::string currentDateTime() {
    std::time_t now = std::time(nullptr);
    std::string dt = std::ctime(&now);
    dt.pop_back(); // 移除换行符
    return dt;
}

// 使用模板和参数包来支持格式化的参数
template <typename... Args>
void log_message(LogLevel level, const std::string& format, Args... args) {
	// 线程池添加任务，每次写日志时用独立线程去写
    log_pool.enqueue([level, format, args...] {
        if (log_file.is_open()) {
            auto log_entry = fmt::format("[{}] [{}] {}\n", currentDateTime(), toString(level), fmt::format(format, args...));
            log_file << log_entry;
        } else {
            std::cerr << "无法打开日志文件！" << std::endl;
        }
    });
}

int main() {
    log_message(INFO, "这是一条信息级别的消息。");
    log_message(ERROR, "错误代码：{}. 错误信息：{}", 404, "未找到");
    // 确保在退出前处理所有日志消息
    // 在真实应用中，这可能会以不同方式处理，例如，在应用关闭时等待所有任务完成。
    std::this_thread::sleep_for(std::chrono::seconds(1)); // 简单的等待日志完成的方式（生产使用不推荐）
    return 0;
}