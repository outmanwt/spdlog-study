# 定义编译器
CXX = g++

# 定义编译选项，-I指定头文件搜索路径
CXXFLAGS = -std=c++11

# 定义链接选项，这里直接指定静态库的完整路径
LDFLAGS = -L../lib -Wl,-rpath,../lib -lfmt

# 定义目标文件名
TARGET = main

# 定义源文件清单
SOURCES = main.cpp

# 默认目标
all: $(TARGET)

# 如何从源文件构建目标
$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) $(LDFLAGS) -o $(TARGET)

# 清理编译生成的文件
clean:
	rm -f $(TARGET)

# 防止文件名与目标冲突
.PHONY: all clean
