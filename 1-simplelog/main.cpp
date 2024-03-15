#include <iostream>
#include <fstream>
#include <string>

void log_message(const std::string& message) {
    std::ofstream log_file("log.txt", std::ios::app);
    if (log_file.is_open()) {
        log_file << message << std::endl;
        log_file.close();
    } else {
        std::cerr << "Failed to open log file!" << std::endl;
    }
}

// 使用示例
int main() {
    log_message("This is a message.");
    return 0;
}