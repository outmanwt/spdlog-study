# 定义编译器
CXX = g++

# 定义编译选项，-I指定头文件搜索路径
CXXFLAGS = -std=c++11

# 定义链接选项，这里直接指定静态库的完整路径
LDFLAGS = -g -L../lib -Wl,-rpath,../lib -lfmt -pthread

# 定义目标文件名
TARGETS = 1 2 3 4

# 默认目标
all: $(TARGETS)

# 通用规则：如何从每个.cpp文件构建对应的目标
%: %.cpp
	$(CXX) $(CXXFLAGS) $< $(LDFLAGS) -o $@

# 清理编译产物
clean:
	rm -f $(TARGETS)

.PHONY: all clean