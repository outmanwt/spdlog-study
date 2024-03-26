#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <fmt/core.h> // 包含 fmt 库的核心功能

static std::ofstream log_file("log.txt", std::ios::app); // 静态日志文件对象

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
    if (log_file.is_open()) {
        auto log_entry = fmt::format("[{}] [{}] {}\n", currentDateTime(), toString(level), fmt::format(format, args...));
        log_file << log_entry;
    } else {
        std::cerr << "Failed to open log file!" << std::endl;
    }
}

int main() {
    log_message(INFO, "This is an info message.");
    log_message(ERROR, "Error code: {}. Error message: {}", 404, "Not Found");
    return 0;
}
