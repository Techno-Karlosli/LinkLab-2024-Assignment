CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I./include -Os -g -fPIE
REQUIRED_CXX_STANDARD = 17

# 源文件
BASE_SRCS = $(shell find src/base -name '*.cpp')
STUDENT_SRCS = $(shell find src/student -name '*.cpp')
HEADERS = $(shell find include -name '*.h' -o -name '*.hpp')

# 所有源文件
SRCS = $(BASE_SRCS) $(STUDENT_SRCS)

# 目标文件
OBJS = $(SRCS:.cpp=.o)

# 基础可执行文件
BASE_EXEC = fle_base

# 工具名称
TOOLS = cc ld nm objdump readfle exec

# 默认目标
all: check_compiler $(TOOLS)

# 检查编译器版本和标准支持
check_compiler:
	@echo "Checking compiler configuration..."
	@echo "Current compiler: $$($(CXX) --version | head -n 1)"
	@if ! $(CXX) -std=c++$(REQUIRED_CXX_STANDARD) -dM -E - < /dev/null > /dev/null 2>&1; then \
		echo "Error: $(CXX) does not support C++$(REQUIRED_CXX_STANDARD)"; \
		exit 1; \
	fi
	@echo "Compiler supports C++$(REQUIRED_CXX_STANDARD) ✓"
	@echo "Compiler check completed"
	@echo "------------------------"

# 编译源文件
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $< -g

# 先编译基础可执行文件
$(BASE_EXEC): $(OBJS) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) -pie

# 为每个工具创建符号链接
$(TOOLS): $(BASE_EXEC)
	@if [ ! -L $@ ] || [ ! -e $@ ]; then \
		ln -sf $(BASE_EXEC) $@; \
	fi

# 清理编译产物
clean:
	rm -f $(OBJS) $(BASE_EXEC) $(TOOLS)
	rm -rf tests/cases/*/build

# 运行测试
test: all
	python3 grader.py

test_1: all
	python3 grader.py --group nm

test_2: all
	python3 grader.py --group basic_linking

test_3: all
	python3 grader.py --group relative_reloc

test_4: all
	python3 grader.py --group symbol_resolution

test_5: all
	python3 grader.py --group addr64

test_6: all
	python3 grader.py --group section_perm

.PHONY: all clean test check_compiler test_1 test_2 test_3 test_4 test_5 test_6
