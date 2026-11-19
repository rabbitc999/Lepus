# Makefile —— Lepus 工具链（手写、纳入版本管理，不再由 HareParse 覆盖）
#
# 与同目录的 `gen.sh` 分工：
#   * `gen.sh` 只重新生成解析器 / 驱动源码（`lepus_parser.*` / `lepus_main.cpp`），
#     它给 HareParse 传 `--no-build-files`，所以不会覆盖这份 Makefile；
#   * 这份 Makefile 负责「生成代码 → 构建」两段：
#       make gen   用 HareParse 从 `lepus.gram` 重新生成源码
#       make       编译静态库 libparser.a 与统一工具链 lepus
#     生成与构建也可以分开：先 `make gen`，再 `make`。
#
# 目标：
#   make        默认编译静态库 libparser.a 与统一工具链 lepus
#   make gen    跑 gen.sh：从 lepus.gram 重新生成解析器 / 驱动源码
#   make lib    只编译静态库 libparser.a
#   make demo   只编译统一工具链 lepus
#   make run    编译并运行（ARGS 会当 argv 传给 lepus）
#   make check  跑 example/samples/*.lep 与 example/expected/*.txt 的自检
#   make clean  删除中间文件与产物
#
# 覆盖变量示例：
#   make CXX=clang++ CXXFLAGS="-std=c++17 -O0 -g -Wall -Wextra"

# 注意：这份工程用 Lepus 自己的 C 分配器（LEPUS_CUSTOM_ALLOCATOR=1，
#       见 local.mk），不是生成模板里那份「标准库内存」的默认值。
# 注意：make 内置定义了 CXX/AR/ARFLAGS，用 ?= 不会生效，这里显式赋值；
# 命令行传参（make CXX=clang++ ...）优先级最高，仍可覆盖。
CXX      = g++
AR       = ar
ifeq ($(LEPUS_STATIC_LLVM),1)
LEPUS_LLVM_SDK ?= build/dependencies/llvm/static-sdk
CXX = $(LEPUS_LLVM_SDK)/bin/g++.exe
CC = $(LEPUS_LLVM_SDK)/bin/gcc.exe
CFLAGS += --sysroot=$(abspath $(LEPUS_LLVM_SDK))
AR = $(LEPUS_LLVM_SDK)/bin/ar.exe
LEPUS_BUILD_ROOT ?= build/llvm-static-gcc16
export PATH := $(abspath $(LEPUS_LLVM_SDK)/bin);$(PATH)
endif
# Release 口径（默认）：关断言 + 按函数/数据分节，配合链接期裁剪与 strip 减小产物体积。
# 需要调试时覆盖：make CXXFLAGS="-std=c++17 -Wall -Wextra -O0 -g" LDFLAGS=""
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -DNDEBUG -ffunction-sections -fdata-sections
# 链接期：--gc-sections 丢弃未引用的节，-s 去掉符号表（体积主要靠这两项下降）。
LDFLAGS  = -Wl,--gc-sections -s
ifeq ($(OS),Windows_NT)
# VSIX 只内嵌编译器 EXE；静态链接 MinGW 运行库，避免依赖用户的 MinGW PATH。
LDFLAGS += -static
endif
LDLIBS   = -lm
export TMPDIR := $(CURDIR)/tmp
export TMP := $(CURDIR)/tmp
export TEMP := $(CURDIR)/tmp
ARFLAGS  = rcs

# lztext：内嵌标准库 / 运行时资产的压缩存储与运行时解压（见 source/lztext/README.md）。
LEPUS_BUILD_ROOT ?= build
export LEPUS_BUILD_ROOT
FRONTEND_BUILD := $(LEPUS_BUILD_ROOT)/lepus-frontend
LIB_SRCS  := source/lepus_parser.cpp source/lztext/lztext.cpp
DEMO_SRCS := source/lepus_main.cpp
HDRS      := source/lepus_parser.hpp source/lepus_parser_user.h
LIB_OBJS  := $(FRONTEND_BUILD)/lepus_parser.o $(FRONTEND_BUILD)/lztext.o
DEMO_OBJS := $(FRONTEND_BUILD)/lepus_main.o

LIB    := $(FRONTEND_BUILD)/libparser.a
include features.mk
LEPUS_VERSION := $(strip $(shell $(PYTHON) -c "from pathlib import Path; print(Path('VERSION').read_text().strip())"))
LEPUS_STATIC_LLVM ?= 0
LEPUS_TARGET_ARCH ?= x64
ifeq ($(OS),Windows_NT)
TARGET := lepus-$(LEPUS_VERSION)-llvm-$(if $(filter 1,$(LEPUS_STATIC_LLVM)),static,external)-windows-$(LEPUS_TARGET_ARCH).exe
else
TARGET := lepus
endif

.PHONY: all lib demo run clean gen check samples reflection llvm llvm-static native-extensions

all: stdlib-embedded

tmp:
	$(PYTHON) -c "from pathlib import Path; Path('tmp').mkdir(parents=True, exist_ok=True)"

# BUILD 的 LLVM 工具及静态 SDK 从锁定上游在线准备，下载缓存不入库。
llvm:
	python library/llvm/setup.py

llvm-static:
	python library/llvm/setup_static.py
	$(MAKE) LEPUS_STATIC_LLVM=1

# 两步执行：先构建归档，再由 stdlib-embedded 在下一次 make 中登记依赖。
native-extensions:
	python tools/setup_native_extensions.py --build-root "$(LEPUS_BUILD_ROOT)" --cc "$(CC)" $(if $(filter 1,$(LEPUS_STATIC_LLVM)),--sysroot "$(LEPUS_LLVM_SDK)") $(LEPUS_NATIVE_EXTENSIONS_FLAGS)

lib: $(LIB)

demo: $(TARGET)

# 重新生成解析器 / 驱动源码（产物位于 source/ 的 lepus_parser.* / lepus_main.cpp）。
gen:
	$(SH) gen.sh

$(LIB): $(LIB_OBJS) | $(FRONTEND_BUILD)
	$(AR) $(ARFLAGS) $@ $^

$(TARGET): $(DEMO_OBJS) $(LIB)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(DEMO_OBJS) $(LIB) $(LDLIBS) $(LEPUS_BUILD_LDLIBS)

$(FRONTEND_BUILD): | tmp
ifeq ($(OS),Windows_NT)
	powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$@' | Out-Null"
else
	mkdir -p $@
endif

$(FRONTEND_BUILD)/%.o: source/%.cpp $(HDRS) | $(FRONTEND_BUILD)
	$(CXX) $(CXXFLAGS) -MMD -MP -iquote . -c $< -o $@

$(FRONTEND_BUILD)/lztext.o: source/lztext/lztext.cpp $(HDRS) | $(FRONTEND_BUILD)
	$(CXX) $(CXXFLAGS) -MMD -MP -iquote . -c $< -o $@

reflection:
	$(MAKE) LEPUS_WITH_REFLECTION=1 RUNTIME_WITH_REFLECTION=1

run: $(TARGET)
	./$(TARGET) $(ARGS)

clean:
	rm -f $(LIB_OBJS) $(DEMO_OBJS) $(LIB) $(TARGET)

# 挂载点：生成目录里如果有一个 local.mk（可以用 `%copy local.mk` 一起带出来），
# 就在所有默认变量与规则之后被包含进来——工程可以在这里补自己的源文件、
# 头文件搜索路径、链接库与额外目标（例如 `check`）。
-include local.mk

override CXXFLAGS += $(LEPUS_TRIM_FLAGS)

# 阶段编译器与发行文件分开：生成资产失败时不覆盖已经交付的编译器。
LEPUS_STDLIB_BOOTSTRAP := $(FRONTEND_BUILD)/lepus-stdlib-bootstrap$(suffix $(TARGET))
$(LEPUS_STDLIB_BOOTSTRAP): $(DEMO_OBJS) $(LIB) $(AST_LIB) $(VM_LIB) $(HOST_LIB) $(PP_LIB)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(DEMO_OBJS) $(LIB) $(LDLIBS) $(LEPUS_BUILD_LDLIBS)

# 先链接独立阶段编译器，再生成确定性核心单元，最后构建发行编译器。
.PHONY: stdlib-embedded
LEPUS_STDLIB_COMPILER ?= $(LEPUS_STDLIB_BOOTSTRAP)
.PHONY: stdlib-units
stdlib-units: $(LEPUS_STDLIB_COMPILER)
	$(PYTHON) tools/build_stdlib_units.py --compiler "$(abspath $(LEPUS_STDLIB_COMPILER))" $(if $(LEPUS_STDLIB_RUNNER),--runner "$(LEPUS_STDLIB_RUNNER)")

stdlib-embedded: stdlib-units
	$(MAKE) $(TARGET)
	$(PYTHON) tools/build_stdlib_units.py --sync-embedded-source "$(LEPUS_EMBED_SOURCE)"
