#include <iostream>
#include <fstream>
#include <string>

enum LogLevel {
    INFO,
    WARNING,
    ERROR
};

void log_message(const std::string& message, LogLevel level) {
    std::ofstream log_file("log.txt", std::ios::app);
    if (log_file.is_open()) {
        switch(level) {
            case INFO:
                log_file << "[INFO] ";
                break;
            case WARNING:
                log_file << "[WARNING] ";
                break;
            case ERROR:
                log_file << "[ERROR] ";
                break;
        }
        log_file << message << std::endl;
        log_file.close();
    } else {
        std::cerr << "Failed to open log file!" << std::endl;
    }
}

// 使用示例
int main() {
    log_message("This is a info message.", LogLevel::INFO);
    return 0;
}