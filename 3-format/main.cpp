#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <fmt/core.h> // 包含 fmt 库的核心功能
#include <fmt/format.h> // 包含 fmt 库的核心功能

enum LogLevel {
    INFO,
    WARNING,
    ERROR
};

// 使用模板和参数包来支持格式化的参数
template <typename... Args>
void log_message(LogLevel level, const std::string& format, Args... args) {
    std::ofstream log_file("log.txt", std::ios::app);
    if (log_file.is_open()) {
        // 获取当前时间
        std::time_t now = std::time(nullptr);
        std::string dt = std::ctime(&now);
        dt.pop_back(); // 移除换行符

        std::string level_str;
        switch(level) {
            case INFO:
                level_str = "INFO";
                break;
            case WARNING:
                level_str = "WARNING";
                break;
            case ERROR:
                level_str = "ERROR";
                break;
        }

        // 使用 fmt::format 格式化日志消息，包括传入的参数
        auto formatted_message = fmt::format(format, args...);
        auto log_entry = fmt::format("[{} {}] {}\n", dt, level_str, formatted_message);

        log_file << log_entry;
        log_file.close();
    } else {
        std::cerr << "Failed to open log file!" << std::endl;
    }
}

// 使用示例
int main() {
    // 使用 INFO 级别记录一条简单消息
    log_message(INFO, "This is an info message.");

    // 使用 ERROR 级别记录一条格式化消息
    log_message(ERROR, "Error code: {}. Error message: {}", 404, "Not Found");

    return 0;
}