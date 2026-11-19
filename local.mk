ifeq ($(origin CC),default)
CC := gcc
endif

# C 入口使用 C11；允许采用 C++17 私有实现，公开调用边界统一为 C API。
ifeq ($(filter -std=%,$(CFLAGS)),)
CFLAGS += -std=c11
endif
CFLAGS += -Isource -O2 -DNDEBUG -ffunction-sections -fdata-sections
CFLAGS += $(LEPUS_TRIM_FLAGS)

# Linux 安装包的宿主对象还会进入 clang 链接的 BUILD PIE。
ifeq ($(LEPUS_RELEASE_PIC),1)
CFLAGS += -fPIC
override CXXFLAGS += -fPIC
endif

# local.mk —— 生成工程自己的挂载点（生成的 Makefile 末尾有 `-include local.mk`）。
#
# lepus 自带 ast / vm / bignum 三份源码副本，只有前处理仍用仓库里的库：
#   * source/ast：已检查 AST 与可选的兼容四元式生成
#   * source/ircode：四元式兼容工具与共享原生运行时（优化交给 LLVM）
#   * source/preprocess：`%include` / `%if` / `%define` 那一步（源码进解析器之前）
# 三份副本各跑 `make -C source/<名字>`（产物仍落在仓库根的 `build/<名字>/`），
# 所有依赖都使用 LEPUS 内部副本。
#
# 注意：`source/{ast,ircode,bignum}` 是从 HareParse 的 `library/` 复制进来的**副本**，
# 这些库副本只在本仓库演进，不修改或同步外部 HareParse 仓库。
#
# 生成源码与报告位于 source/，对象文件与静态库位于 $(LEPUS_BUILD_ROOT)/lepus-frontend/。
# 根目录只保留工具链入口与手写构建文件，重跑 gen.sh 不覆盖 local.mk。

AST_DIR := source/ast
VM_DIR  := source/ircode
PP_DIR  := source/preprocess
AST_LIB := $(LEPUS_BUILD_ROOT)/lepus-runtime/libast.a
VM_BUILD := $(LEPUS_BUILD_ROOT)/lepus-ircode-aot-core
HOST_BUILD := $(LEPUS_BUILD_ROOT)/lepus-host-records
HOST_LIB := $(HOST_BUILD)/librecords.a
CAS_BUILD := $(LEPUS_BUILD_ROOT)/lepus-cas
CAS_SOURCE_NAMES := $(shell $(PYTHON) tools/cas_sources.py)
CAS_OBJS := $(addprefix $(CAS_BUILD)/,$(patsubst %.c,%.o,$(CAS_SOURCE_NAMES:.cpp=.o)))
IPAGE_OBJS := $(HOST_BUILD)/ipage_sql.o $(HOST_BUILD)/ipage_dialect.o
ifneq ($(filter 1,$(NO_LEPUS_STD) $(NO_LEPUS_STD_CAS) $(NO_LEPUS_BIG_INT)),)
CAS_OBJS :=
endif
VM_LIB  := $(VM_BUILD)/libstackvm.a
VM_SOURCE_NAMES := $(shell $(PYTHON) tools/stackvm_sources.py --aot-only)
VM_OBJS := $(addprefix $(VM_BUILD)/,$(VM_SOURCE_NAMES:.cpp=.o))
PP_LIB  := $(LEPUS_BUILD_ROOT)/lepus-runtime/libpreprocess.a

ifeq ($(OS),Windows_NT)
LEPUS_RUNTIME_MKDIR = powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$@' | Out-Null"
else
LEPUS_RUNTIME_MKDIR = mkdir -p $@
endif

LEPUS_SOURCE_DIR  := source
LEPUS_LIBRARY_DIR := library
LEPUS_DEPENDENCY_DIR := build/dependencies
LEPUS_DEPENDENCY_STAMP := $(LEPUS_DEPENDENCY_DIR)/.verified
$(LEPUS_DEPENDENCY_STAMP): tools/fetch_dependencies.py tools/dependencies.lock.json
	$(PYTHON) tools/fetch_dependencies.py --dest $(LEPUS_DEPENDENCY_DIR)
	$(PYTHON) -c "from pathlib import Path; Path(r'$@').touch()"
$(LEPUS_DEPENDENCY_DIR)/sqlite/sqlite3.c $(LEPUS_DEPENDENCY_DIR)/sqlite/sqlite3.h $(LEPUS_DEPENDENCY_DIR)/miniz/miniz_tinfl.c $(LEPUS_DEPENDENCY_DIR)/miniz/miniz_tinfl.h $(LEPUS_DEPENDENCY_DIR)/yyjson/src/yyjson.c $(LEPUS_DEPENDENCY_DIR)/yyjson/src/yyjson.h $(LEPUS_DEPENDENCY_DIR)/tomlc17/src/lepus_tomlc17.c $(LEPUS_DEPENDENCY_DIR)/tomlc17/src/tomlc17.h: $(LEPUS_DEPENDENCY_STAMP)
	@$(PYTHON) tools/fetch_dependencies.py --dest $(LEPUS_DEPENDENCY_DIR)
# 代码里的裸文件名 include 靠这组搜索路径解析：搬进 source/ 的九个库走 LEPUS_SOURCE_DIR，
# caspp / ipage 走 LEPUS_LIBRARY_DIR；SQLite / miniz 从构建缓存读取。
LEPUS_INCLUDES := -I$(LEPUS_SOURCE_DIR) -I$(LEPUS_LIBRARY_DIR) \
    -I$(LEPUS_SOURCE_DIR)/allocator -I$(LEPUS_SOURCE_DIR)/lsp \
    -I$(LEPUS_SOURCE_DIR)/bignum -I$(LEPUS_SOURCE_DIR)/json -I$(LEPUS_SOURCE_DIR)/toml \
    -I$(LEPUS_DEPENDENCY_DIR)/sqlite -I$(LEPUS_DEPENDENCY_DIR)/miniz -I$(LEPUS_SOURCE_DIR)/console -I$(LEPUS_SOURCE_DIR)/lztext
override CXXFLAGS += -DRUNTIME_BUILD_ONLY=1 -DLEPUS_CUSTOM_ALLOCATOR=1 -pthread $(LEPUS_INCLUDES) -I$(AST_DIR) -I$(VM_DIR) -I$(PP_DIR)

# ---- 统一工具链：lepus 同时是前端 / VM / 工程生成器 ----
# `$(FRONTEND_BUILD)/lepus_main.o` 只提供 `lepus_main_entry`，真正的 main 在 lepus_tool.cpp；
# lepus-generator 去掉自己的 main，改成进程内 `runCli`。
override CXXFLAGS += -DLEPUS_FRONT_END_NO_MAIN=1

LEPUS_TOOL_OBJS := $(FRONTEND_BUILD)/lepus_tool.o $(FRONTEND_BUILD)/generator_cli.o
DEMO_OBJS += $(LEPUS_TOOL_OBJS)
$(TARGET): $(LEPUS_TOOL_OBJS)

$(LEPUS_TOOL_OBJS): | $(FRONTEND_BUILD)

# 宿主只读取内嵌资产声明，压缩数据与解压缓存在独立编译单元中。
$(FRONTEND_BUILD)/lepus_main.o $(FRONTEND_BUILD)/lepus_tool.o $(FRONTEND_BUILD)/generator_cli.o: source/lepus_module.hpp source/lepus_stdlib.hpp

$(FRONTEND_BUILD)/lepus_tool.o: source/lepus_tool.cpp source/lepus_package.hpp tools/abi_import.hpp source/generator/generator_cli.hpp \
        source/lepus_runtime_allocator.hpp source/allocator/lepus_allocator.hpp
	$(CXX) $(CXXFLAGS) -MMD -MP -iquote . -c $< -o $@

$(FRONTEND_BUILD)/generator_cli.o: source/generator/generator_cli.cpp source/generator/generator_cli.hpp \
        source/template/engine.hpp source/lepus_runtime_allocator.hpp
	$(CXX) $(CXXFLAGS) -MMD -MP -DLEPUS_GENERATOR_NO_MAIN=1 -iquote . -c $< -o $@

# 注意顺序：libparser.a 引用了 ast / stackvm 的符号，必须排在它们前面。
# 直通 ABI 桥接在宿主胶水库里引用 VM 核心的上下文与堆接口，两者互相引用，
# 用 start-group 保证静态库单遍扫描也能解全部符号。
LDLIBS   += $(AST_LIB) -Wl,--start-group $(VM_LIB) $(HOST_LIB) -Wl,--end-group $(PP_LIB)
ifeq ($(shell uname -s),Linux)
LDLIBS   += -ldl
endif

# `%main` / `%code` / `%epilogue` 的 include 会被生成器展开进 .cpp；显式列出原文件，
# 避免只改泛型 / 宏展开头或降级实现时 make 误判为无需重编。
$(FRONTEND_BUILD)/lepus_tool.o $(FRONTEND_BUILD)/lepus_main.o: source/lepus_project.hpp source/lepus_cli.hpp

$(FRONTEND_BUILD)/lepus_main.o: source/lepus_analysis.hpp source/lepus_diagnostics.hpp source/lepus_stdlib.hpp source/lepus_package.hpp source/lepus_driver.cpp source/lepus_incremental.hpp source/lepus_module.hpp source/macro/signature.hpp source/syntax/source_text.hpp source/lepus_lambda.hpp source/lepus_generic_closure.hpp source/lepus_generic.hpp $(LEPUS_DEPENDENCY_DIR)/sqlite/sqlite3.h $(VM_DIR)/stackvm.hpp $(VM_DIR)/value_text.hpp $(VM_DIR)/reflection.hpp
$(FRONTEND_BUILD)/lepus_parser.o: source/lepus_ast.hpp source/lepus_semantics.cpp source/lepus_generic.hpp $(wildcard source/semantics/*.cpp) $(LEPUS_DEPENDENCY_DIR)/sqlite/sqlite3.h $(VM_DIR)/stackvm.hpp $(VM_DIR)/value_text.hpp $(VM_DIR)/reflection.hpp

# 外部静态库更新后必须重链；只把它们写进 LDLIBS 不会让 make 追踪时间戳。
$(TARGET): $(AST_LIB) $(VM_LIB) $(HOST_LIB) $(PP_LIB)

# Lepus's VM copy includes the std bindings that are intentionally absent from
# the general-purpose source/ircode build. Keep its archive separate.
$(VM_BUILD) $(HOST_BUILD): | tmp
	$(LEPUS_RUNTIME_MKDIR)

$(VM_BUILD)/%.o: $(VM_DIR)/%.cpp | $(VM_BUILD)
	$(CXX) $(CXXFLAGS) -MMD -MP -I$(VM_DIR) -c $< -o $@
-include $(wildcard $(VM_BUILD)/*.d)

$(HOST_BUILD)/bigint.o: source/bignum/bigint.cpp source/bignum/bigint.hpp | $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -MMD -MP -I$(LEPUS_LIBRARY_DIR)/bignum -c $< -o $@

$(HOST_BUILD)/bigfloat.o: source/bignum/bigfloat.cpp source/bignum/bigfloat.hpp source/bignum/bigint.hpp | $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -MMD -MP -I$(LEPUS_LIBRARY_DIR)/bignum -c $< -o $@

BIGINT_C_SRCS := $(wildcard source/bignum/lepus_big*.c)
BIGINT_MODULE_OBJS := $(patsubst source/bignum/%.c,$(HOST_BUILD)/%.o,$(BIGINT_C_SRCS))
$(filter-out $(HOST_BUILD)/runtime_%,$(BIGINT_MODULE_OBJS)): $(HOST_BUILD)/%.o: source/bignum/%.c $(LEPUS_BUILD_ROOT)/lepus-runtime/buildConfig.json | $(HOST_BUILD)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@
RUNTIME_C_SRCS := $(wildcard source/runtime_c/*.c)
RUNTIME_C_OBJS := $(patsubst source/runtime_c/%.c,$(HOST_BUILD)/runtime_%.o,$(RUNTIME_C_SRCS))
$(RUNTIME_C_OBJS): $(LEPUS_BUILD_ROOT)/lepus-runtime/buildConfig.json
$(HOST_BUILD)/runtime_%.o: source/runtime_c/%.c $(LEPUS_DEPENDENCY_DIR)/yyjson/src/yyjson.h $(LEPUS_DEPENDENCY_DIR)/tomlc17/src/tomlc17.h | $(HOST_BUILD)
	$(CC) $(CFLAGS) -I$(LEPUS_DEPENDENCY_DIR)/yyjson/src -I$(LEPUS_DEPENDENCY_DIR)/tomlc17/src -MMD -MP -c $< -o $@
$(HOST_BUILD)/tomlc17.o: $(LEPUS_DEPENDENCY_DIR)/tomlc17/src/lepus_tomlc17.c $(LEPUS_DEPENDENCY_DIR)/tomlc17/src/tomlc17.h $(LEPUS_BUILD_ROOT)/lepus-runtime/buildConfig.json | $(HOST_BUILD)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(HOST_BUILD)/yyjson.o: $(LEPUS_DEPENDENCY_DIR)/yyjson/src/yyjson.c $(LEPUS_DEPENDENCY_DIR)/yyjson/src/yyjson.h $(LEPUS_BUILD_ROOT)/lepus-runtime/buildConfig.json | $(HOST_BUILD)
	$(CC) $(CFLAGS) -DYYJSON_DISABLE_FILE=1 -DYYJSON_DISABLE_WRITER=1 -DYYJSON_DISABLE_UTILS=1 -DYYJSON_DISABLE_INCR_READER=1 -DYYJSON_READER_DEPTH_LIMIT=256 -MMD -MP -c $< -o $@

-include $(BIGINT_MODULE_OBJS:.o=.d) $(HOST_BUILD)/bigint.d $(HOST_BUILD)/bigfloat.d

BIGFLOAT_MATH_OBJS := $(HOST_BUILD)/bigfloat_roots.o $(HOST_BUILD)/bigfloat_transcendental.o $(HOST_BUILD)/bigfloat_trigonometry.o
$(BIGFLOAT_MATH_OBJS): $(HOST_BUILD)/%.o: source/bignum/%.cpp $(LEPUS_BUILD_ROOT)/lepus-runtime/buildConfig.json | $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -MMD -MP -I$(LEPUS_LIBRARY_DIR)/bignum -c $< -o $@

$(HOST_BUILD)/sqlite3.o: $(LEPUS_DEPENDENCY_DIR)/sqlite/sqlite3.c $(LEPUS_DEPENDENCY_DIR)/sqlite/sqlite3.h $(LEPUS_BUILD_ROOT)/lepus-runtime/buildConfig.json | $(HOST_BUILD)
	$(CC) $(CFLAGS) -std=c11 -O2 -DSQLITE_THREADSAFE=1 -DSQLITE_OMIT_LOAD_EXTENSION=1 -c $< -o $@

$(VM_LIB): $(VM_OBJS)
	$(PYTHON) -c "from pathlib import Path; Path(r'$@').unlink(missing_ok=True)"
	$(AR) $(ARFLAGS) $@ $^

# 每个标准库模块独立编译；实际包含关系由编译器生成 .d 文件追踪。
LEPUS_STANDARD_SRCS := $(wildcard source/abi/std_*.cpp)
LEPUS_STANDARD_OBJS := $(patsubst source/abi/%.cpp,$(HOST_BUILD)/%.o,$(LEPUS_STANDARD_SRCS))
$(HOST_BUILD)/std_%.o: source/abi/std_%.cpp | $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -MMD -MP -iquote . -c $< -o $@
$(HOST_BUILD)/abi_abi_cas.o: private CXXFLAGS += -Ilibrary/caspp/include
$(HOST_BUILD)/lztext_kernel.o: source/lztext/lztext.cpp source/lztext/lztext.hpp $(LEPUS_BUILD_ROOT)/lepus-runtime/buildConfig.json | $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -MMD -MP -iquote . -c $< -o $@
$(HOST_BUILD)/json.o: source/json/json.cpp | $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -MMD -MP -iquote . -c $< -o $@
$(HOST_BUILD)/toml.o: source/toml/toml.cpp | $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -MMD -MP -iquote . -c $< -o $@
$(HOST_BUILD)/toml_source.o: source/toml/toml_source.cpp | $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -MMD -MP -iquote . -c $< -o $@
$(LEPUS_STANDARD_OBJS) $(HOST_BUILD)/json.o $(HOST_BUILD)/toml.o $(HOST_BUILD)/toml_source.o: $(LEPUS_BUILD_ROOT)/lepus-runtime/buildConfig.json
-include $(wildcard $(HOST_BUILD)/*.d)

$(CAS_BUILD)/%.o: library/caspp/%.cpp
	$(PYTHON) -c "from pathlib import Path; Path(r'$(@D)').mkdir(parents=True, exist_ok=True)"
	$(CXX) $(CXXFLAGS) -Ilibrary/caspp/include -MMD -MP -c $< -o $@
$(CAS_BUILD)/%.o: library/caspp/%.c
	@$(PYTHON) -c "from pathlib import Path; Path('$(dir $@)').mkdir(parents=True, exist_ok=True)"
	$(CC) $(CFLAGS) -std=c11 -Ilibrary/caspp/include -Isource/bignum -Isource/runtime_c -MMD -MP -c $< -o $@
-include $(shell $(PYTHON) -c "from pathlib import Path; print(' '.join(str(p).replace(chr(92), '/') for p in Path('$(CAS_BUILD)').rglob('*.d')))")
$(CAS_OBJS): $(LEPUS_BUILD_ROOT)/lepus-runtime/buildConfig.json

$(HOST_BUILD)/ipage_%.o: library/ipage/source/ipage/%.cpp | $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -MMD -MP -iquote . -c $< -o $@
$(IPAGE_OBJS): $(LEPUS_BUILD_ROOT)/lepus-runtime/buildConfig.json

# 直通 ABI：C 胶水按模块分文件编译，命名沿用 abi_*.c。
LEPUS_ABI_C_SRCS := $(wildcard source/abi/*.c)
LEPUS_ABI_CXX_SRCS := $(filter-out $(LEPUS_STANDARD_SRCS),$(wildcard source/abi/*.cpp))
LEPUS_ABI_OBJS := $(patsubst source/abi/%.c,$(HOST_BUILD)/abi_%.o,$(LEPUS_ABI_C_SRCS)) \
                     $(patsubst source/abi/%.cpp,$(HOST_BUILD)/abi_%.o,$(LEPUS_ABI_CXX_SRCS))
$(HOST_BUILD)/abi_%.o: source/abi/%.c | $(HOST_BUILD)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@
$(HOST_BUILD)/abi_%.o: source/abi/%.cpp | $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -MMD -MP -iquote . -c $< -o $@
$(LEPUS_ABI_OBJS): $(LEPUS_BUILD_ROOT)/lepus-runtime/buildConfig.json
$(HOST_BUILD)/abi_abi_sql.o: $(LEPUS_DEPENDENCY_DIR)/sqlite/sqlite3.h

$(HOST_LIB): $(RUNTIME_C_OBJS) $(HOST_BUILD)/yyjson.o $(HOST_BUILD)/tomlc17.o $(HOST_BUILD)/lztext_kernel.o $(BIGINT_MODULE_OBJS) $(HOST_BUILD)/sqlite3.o $(HOST_BUILD)/bigint.o $(HOST_BUILD)/bigfloat.o $(BIGFLOAT_MATH_OBJS) $(LEPUS_STANDARD_OBJS) $(HOST_BUILD)/json.o $(HOST_BUILD)/toml.o $(HOST_BUILD)/toml_source.o $(CAS_OBJS) $(IPAGE_OBJS) $(LEPUS_ABI_OBJS)
	$(PYTHON) -c "from pathlib import Path; Path(r'$@').unlink(missing_ok=True)"
	$(AR) $(ARFLAGS) $@ $^

.PHONY: check samples

# Windows 上 make 找不到 sh：优先用 Git Bash / MSYS2 的 bash 跑 check.sh（与 project/mini-basic 的写法一致）。
ifeq ($(OS),Windows_NT)
  SH ?= $(firstword $(wildcard C:/PROGRA~1/Git/bin/bash.exe) \
                    $(wildcard C:/PROGRA~2/Git/bin/bash.exe) \
                    $(wildcard C:/msys64/usr/bin/bash.exe) sh)
else
  SH ?= sh
endif

# 自检：跑 example/samples/*.lep，逐个比对 example/expected/*.txt。
check: $(TARGET)
	$(SH) check.sh

# 只跑样例、不重新编译（改样例时用）。
samples: $(TARGET)
	$(SH) check.sh

# Keep allocator-specialized dependencies separate from the generic libraries.
$(LEPUS_BUILD_ROOT)/lepus-runtime: | tmp
	$(LEPUS_RUNTIME_MKDIR)
AST_OBJS := $(LEPUS_BUILD_ROOT)/lepus-runtime/ast.o $(LEPUS_BUILD_ROOT)/lepus-runtime/ast_type.o $(LEPUS_BUILD_ROOT)/lepus-runtime/ast_node.o $(LEPUS_BUILD_ROOT)/lepus-runtime/ast_data.o
$(AST_OBJS): $(LEPUS_BUILD_ROOT)/lepus-runtime/%.o: $(AST_DIR)/%.cpp $(LEPUS_BUILD_ROOT)/lepus-runtime/buildConfig.json | $(LEPUS_BUILD_ROOT)/lepus-runtime
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@
-include $(AST_OBJS:.o=.d)
$(LEPUS_BUILD_ROOT)/lepus-runtime/preprocess.o: $(PP_DIR)/preprocess.cpp $(PP_DIR)/preprocess.hpp source/allocator/lepus_allocator.hpp source/allocator/lepus_pool.c | $(LEPUS_BUILD_ROOT)/lepus-runtime
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(AST_LIB): $(AST_OBJS)
	$(AR) $(ARFLAGS) $@ $^
$(PP_LIB): $(LEPUS_BUILD_ROOT)/lepus-runtime/preprocess.o
	$(AR) $(ARFLAGS) $@ $^
$(FRONTEND_BUILD)/lepus_main.o $(FRONTEND_BUILD)/lepus_parser.o $(VM_OBJS) $(HOST_BUILD)/bigint.o $(HOST_BUILD)/bigfloat.o: source/lepus_runtime_allocator.hpp source/allocator/lepus_allocator.hpp source/allocator/lepus_pool.c source/allocator/lepus_pool.h

.PHONY: config-force
$(LEPUS_BUILD_ROOT)/lepus-runtime/buildConfig.json: config-force
	$(PYTHON) tools/write_build_config.py $@ "$(CXX)" "$(CXXFLAGS)" "$(LDLIBS)" "$(LDFLAGS)"
$(LIB_OBJS) $(DEMO_OBJS) $(VM_OBJS) $(HOST_BUILD)/bigint.o $(HOST_BUILD)/bigfloat.o $(LEPUS_BUILD_ROOT)/lepus-runtime/ast.o $(LEPUS_BUILD_ROOT)/lepus-runtime/preprocess.o: $(LEPUS_BUILD_ROOT)/lepus-runtime/buildConfig.json

# JSON 与 TOML 实现由宿主库单独编译，包含依赖由 .d 文件维护。

# ---- Lepus 宏/块 LSP（source/lsp）：编进编译器本体，不再另发可执行文件 --------------
# `lepus lsp --macros` 在**本进程内**跑它（见 lepus_tool.cpp 的分派）；共用件
# （最小 JSON 与头文件级共用类型）本仓库自带：`source/json/` 与 `source/lsp/protocol_types.hpp`，不再引用外面。
LSP_DIR := source/lsp
LSP_LIB_DIR := source/lsp
EMBEDDED_LSP_BUILD := $(LEPUS_BUILD_ROOT)/lepus-embedded-lsp

$(EMBEDDED_LSP_BUILD):
	$(LEPUS_RUNTIME_MKDIR)
$(EMBEDDED_LSP_BUILD)/macro_analyzer.o: $(LSP_DIR)/macro_analyzer.cpp $(LSP_DIR)/macro_analyzer.hpp $(LSP_LIB_DIR)/protocol_types.hpp source/macro/signature.hpp source/syntax/source_text.hpp | $(EMBEDDED_LSP_BUILD)
	$(CXX) $(CXXFLAGS) -MMD -MP -iquote . -c $< -o $@
$(EMBEDDED_LSP_BUILD)/language_server.o: $(LSP_DIR)/language_server.cpp $(LSP_DIR)/language_server.hpp $(LSP_DIR)/macro_analyzer.hpp source/json/json.hpp | $(EMBEDDED_LSP_BUILD)
	$(CXX) $(CXXFLAGS) -MMD -MP -iquote . -c $< -o $@
LEPUS_LSP_OBJS := $(EMBEDDED_LSP_BUILD)/macro_analyzer.o $(EMBEDDED_LSP_BUILD)/language_server.o
DEMO_OBJS += $(LEPUS_LSP_OBJS)
$(TARGET): $(LEPUS_LSP_OBJS)

# 静态消息表变更后所有相关翻译单元必须一致，避免旧消息表混入链接。
$(LIB_OBJS) $(DEMO_OBJS) $(VM_OBJS): source/lepus_i18n.hpp source/console/utf8.hpp

# 单态化符号由前端与宿主胶水共享；更新编码时必须重编相关目标。
$(FRONTEND_BUILD)/lepus_main.o $(FRONTEND_BUILD)/lepus_parser.o: source/lepus_generic_name.hpp
$(FRONTEND_BUILD)/lepus_main.o $(FRONTEND_BUILD)/lepus_parser.o: source/lepus_lambda.hpp source/lepus_report.hpp

$(FRONTEND_BUILD)/lepus_parser.o $(FRONTEND_BUILD)/lepus_main.o: source/lepus_borrow_checker.hpp

$(FRONTEND_BUILD)/lepus_parser.o $(FRONTEND_BUILD)/lepus_main.o: source/lepus_borrow_flow.hpp

# 原生语言服务和开发命令。
LEPUS_NATIVE_DEVELOPMENT := $(LEPUS_BUILD_ROOT)/lepus-native-development.o
DEMO_OBJS += $(LEPUS_NATIVE_DEVELOPMENT)
$(TARGET): $(LEPUS_NATIVE_DEVELOPMENT)
$(LEPUS_BUILD_ROOT)/lepus-native: | tmp
	$(LEPUS_RUNTIME_MKDIR)
$(LEPUS_NATIVE_DEVELOPMENT): tools/native_development.cpp tools/native_development.hpp source/syntax/source_text.hpp source/lepus_module.hpp source/lepus_stdlib.hpp source/lepus_package.hpp $(LEPUS_BUILD_ROOT)/lepus-runtime/buildConfig.json | $(LEPUS_BUILD_ROOT)/lepus-native
	$(CXX) $(CXXFLAGS) -MMD -MP -MF $(LEPUS_BUILD_ROOT)/lepus-native-development.d -iquote . -c $< -o $@
-include $(LEPUS_BUILD_ROOT)/lepus-native-development.d
$(LEPUS_NATIVE_DEVELOPMENT): source/lepus_i18n.hpp source/console/utf8.hpp
$(FRONTEND_BUILD)/lepus_tool.o: tools/native_development.hpp

# 内置 ZIP/Gzip 的 Deflate 解压器；仅构建解压部分。
LEPUS_MINIZ := $(LEPUS_BUILD_ROOT)/lepus-miniz-tinfl.o
LIB_OBJS += $(LEPUS_MINIZ)
$(LIB): $(LEPUS_MINIZ)
$(LEPUS_MINIZ): $(LEPUS_DEPENDENCY_DIR)/miniz/miniz_tinfl.c $(LEPUS_DEPENDENCY_DIR)/miniz/miniz_tinfl.h | $(LEPUS_BUILD_ROOT)/lepus-native
	$(CC) $(CFLAGS) -O2 -DMINIZ_NO_ARCHIVE_APIS -DMINIZ_NO_DEFLATE_APIS -c $< -o $@
ifeq ($(OS),Windows_NT)
LDLIBS += -lwinhttp
endif

-include $(LEPUS_BUILD_ROOT)/lepus-native-development.d

# 泛型与闭包文本处理实现独立编译，解析器和语言服务只依赖声明。
LEPUS_FRONTEND_MODULES := lepus_resource_cleanup lepus_generic lepus_lambda lepus_generic_closure lepus_package lepus_report lepus_aot lepus_aot_target lepus_aot_debug lepus_aot_llvm lepus_aot_runtime_assets lepus_temp lepus_digest lepus_library_bitcode lepus_library_manifest lepus_library_templates lepus_library_package lepus_library_tools lepus_native_tools lepus_native_objects lepus_linker lepus_archive_llvm lepus_assembler lepus_assembler_llvm
LEPUS_FRONTEND_MODULE_OBJS := $(addprefix $(FRONTEND_BUILD)/,$(addsuffix .o,$(LEPUS_FRONTEND_MODULES)))
LIB_OBJS += $(LEPUS_FRONTEND_MODULE_OBJS)
$(LIB): $(LEPUS_FRONTEND_MODULE_OBJS)
$(LEPUS_FRONTEND_MODULE_OBJS): $(FRONTEND_BUILD)/%.o: source/%.cpp | $(FRONTEND_BUILD)
	$(CXX) $(CXXFLAGS) -MMD -MP -iquote . -c $< -o $@
$(LEPUS_FRONTEND_MODULE_OBJS): $(LEPUS_BUILD_ROOT)/lepus-runtime/buildConfig.json
-include $(wildcard $(FRONTEND_BUILD)/*.d)

# 静态 LLVM 变体只替换后端隔离单元，使用匹配 GCC 并隔离全部对象文件。
LEPUS_STATIC_LLVM ?= 0
LEPUS_LLVM_SDK ?= build/dependencies/llvm/static-sdk
$(LEPUS_BUILD_ROOT)/lepus-native/llvmLinkMode.json: config-force
	$(PYTHON) tools/write_build_config.py $@ "$(LEPUS_STATIC_LLVM)" "$(LEPUS_LLVM_SDK)" "$(LEPUS_BUILD_LDLIBS)"
$(TARGET): $(LEPUS_BUILD_ROOT)/lepus-native/llvmLinkMode.json
ifeq ($(LEPUS_STATIC_LLVM),1)
ifneq ($(OS),Windows_NT)
$(error 当前静态 LLVM 分发仅验收 Windows x64)
endif
override CXXFLAGS += --sysroot=$(abspath $(LEPUS_LLVM_SDK))
LDFLAGS += --sysroot=$(abspath $(LEPUS_LLVM_SDK)) -fuse-ld=lld
LEPUS_LLVM_CONFIG := $(LEPUS_LLVM_SDK)/bin/llvm-config.exe
ifeq ($(wildcard $(LEPUS_LLVM_CONFIG)),)
$(error 请先执行 python library/llvm/setup_static.py 恢复静态 LLVM 开发库)
endif
LEPUS_LLVM_STATIC_OBJ := $(LEPUS_BUILD_ROOT)/lepus-native/lepus_aot_llvm_static.o
LEPUS_LLVM_COMPONENTS := core irreader passes x86codegen x86asmparser dtlto libdriver lto option windowsdriver windowsmanifest all-targets
LEPUS_LLVM_LIBS := $(shell $(LEPUS_LLVM_CONFIG) --link-static --libs $(LEPUS_LLVM_COMPONENTS))
LEPUS_LLVM_SYSTEM_LIBS := $(subst -lzstd.dll,-lzstd,$(shell $(LEPUS_LLVM_CONFIG) --link-static --system-libs $(LEPUS_LLVM_COMPONENTS))) -liconv -lbcrypt
DEMO_OBJS += $(LEPUS_LLVM_STATIC_OBJ)
LEPUS_LLVM_IMPORT_OBJ := $(LEPUS_BUILD_ROOT)/lepus-native/llvm_static_imports.o
DEMO_OBJS += $(LEPUS_LLVM_IMPORT_OBJ)
$(TARGET): $(LEPUS_LLVM_IMPORT_OBJ)
$(LEPUS_LLVM_IMPORT_OBJ): tools/llvm_static_imports.cpp | $(LEPUS_BUILD_ROOT)/lepus-native
	$(CXX) $(CXXFLAGS) -I$(LEPUS_LLVM_SDK)/include/libxml2 -MMD -MP -c $< -o $@
$(TARGET): $(LEPUS_LLVM_STATIC_OBJ)
LEPUS_BUILD_LDLIBS := -L$(LEPUS_LLVM_SDK)/lib -Wl,--start-group -llldMinGW -llldCOFF -llldELF -llldCommon $(LEPUS_LLVM_LIBS) $(LEPUS_LLVM_SYSTEM_LIBS) -Wl,--end-group
$(LEPUS_LLVM_STATIC_OBJ): source/lepus_aot_llvm.cpp source/lepus_aot_llvm.hpp source/lepus_aot.hpp $(LEPUS_LLVM_CONFIG) | $(LEPUS_BUILD_ROOT)/lepus-native
	$(CXX) $(CXXFLAGS) -DLEPUS_STATIC_LLVM=1 -DLLVM_BUILD_STATIC -I$(LEPUS_LLVM_SDK)/include -Ilibrary -MMD -MP -c $< -o $@
-include $(LEPUS_BUILD_ROOT)/lepus-native/lepus_aot_llvm_static.d
LEPUS_ASSEMBLER_STATIC_OBJ := $(LEPUS_BUILD_ROOT)/lepus-native/lepus_assembler_llvm_static.o
DEMO_OBJS += $(LEPUS_ASSEMBLER_STATIC_OBJ)
$(TARGET): $(LEPUS_ASSEMBLER_STATIC_OBJ)
$(LEPUS_ASSEMBLER_STATIC_OBJ): source/lepus_assembler_llvm.cpp source/lepus_assembler.hpp source/lepus_aot.hpp $(LEPUS_LLVM_CONFIG) | $(LEPUS_BUILD_ROOT)/lepus-native
	$(CXX) $(CXXFLAGS) -DLEPUS_STATIC_LLVM=1 -DLLVM_BUILD_STATIC -I$(LEPUS_LLVM_SDK)/include -Ilibrary -MMD -MP -c $< -o $@
-include $(LEPUS_BUILD_ROOT)/lepus-native/lepus_assembler_llvm_static.d
LEPUS_ARCHIVE_STATIC_OBJ := $(LEPUS_BUILD_ROOT)/lepus-native/lepus_archive_llvm_static.o
DEMO_OBJS += $(LEPUS_ARCHIVE_STATIC_OBJ)
$(TARGET): $(LEPUS_ARCHIVE_STATIC_OBJ)
$(LEPUS_ARCHIVE_STATIC_OBJ): source/lepus_archive_llvm.cpp source/lepus_linker.hpp source/lepus_aot.hpp $(LEPUS_LLVM_CONFIG) | $(LEPUS_BUILD_ROOT)/lepus-native
	$(CXX) $(CXXFLAGS) -DLEPUS_STATIC_LLVM=1 -DLLVM_BUILD_STATIC -I$(LEPUS_LLVM_SDK)/include -Ilibrary -MMD -MP -c $< -o $@
-include $(LEPUS_BUILD_ROOT)/lepus-native/lepus_archive_llvm_static.d


endif

# 编译期 AST 宏的独立实现，避免 regex 等重型头传播到解析器接口。
LEPUS_MACRO_MODULES := hygiene regex_matcher source fragments ast_substitution template_binding registry signature
LEPUS_MACRO_OBJS := $(addprefix $(FRONTEND_BUILD)/macro_,$(addsuffix .o,$(LEPUS_MACRO_MODULES)))
LIB_OBJS += $(LEPUS_MACRO_OBJS)
$(LIB): $(LEPUS_MACRO_OBJS)
$(LEPUS_MACRO_OBJS): $(FRONTEND_BUILD)/macro_%.o: source/macro/%.cpp | $(FRONTEND_BUILD)
	$(CXX) $(CXXFLAGS) -MMD -MP -iquote . -c $< -o $@
$(LEPUS_MACRO_OBJS): $(LEPUS_BUILD_ROOT)/lepus-runtime/buildConfig.json

LEPUS_LITERAL_OBJ := $(FRONTEND_BUILD)/string_escape.o
LIB_OBJS += $(LEPUS_LITERAL_OBJ)
$(LIB): $(LEPUS_LITERAL_OBJ)
$(LEPUS_LITERAL_OBJ): source/literals/string_escape.cpp source/literals/string_escape.hpp | $(FRONTEND_BUILD)
	$(CXX) $(CXXFLAGS) -MMD -MP -iquote . -c $< -o $@

LEPUS_EDITOR_ANALYSIS_OBJ := $(FRONTEND_BUILD)/compiler_analysis.o
LIB_OBJS += $(LEPUS_EDITOR_ANALYSIS_OBJ)
$(LIB): $(LEPUS_EDITOR_ANALYSIS_OBJ)
$(LEPUS_EDITOR_ANALYSIS_OBJ): source/editor/compiler_analysis.cpp source/editor/compiler_analysis.hpp | $(FRONTEND_BUILD)
	$(CXX) $(CXXFLAGS) -MMD -MP -iquote . -c $< -o $@

LEPUS_SYNTAX_MODULES := source_text constant_expression
LEPUS_SYNTAX_OBJS := $(addprefix $(FRONTEND_BUILD)/syntax_,$(addsuffix .o,$(LEPUS_SYNTAX_MODULES)))
LIB_OBJS += $(LEPUS_SYNTAX_OBJS)
$(LIB): $(LEPUS_SYNTAX_OBJS)
$(LEPUS_SYNTAX_OBJS): $(FRONTEND_BUILD)/syntax_%.o: source/syntax/%.cpp | $(FRONTEND_BUILD)
	$(CXX) $(CXXFLAGS) -MMD -MP -iquote . -c $< -o $@
$(LEPUS_LITERAL_OBJ): $(LEPUS_BUILD_ROOT)/lepus-runtime/buildConfig.json

LEPUS_EMBED_SOURCE := $(FRONTEND_BUILD)/lepus_stdlib_embedded.cpp
LEPUS_STDLIB_OBJ := $(FRONTEND_BUILD)/lepus_stdlib_embedded.o
LIB_OBJS += $(LEPUS_STDLIB_OBJ)
$(LIB): $(LEPUS_STDLIB_OBJ)
$(LEPUS_STDLIB_OBJ): $(LEPUS_EMBED_SOURCE) source/lepus_stdlib.hpp | $(FRONTEND_BUILD)
	$(CXX) $(CXXFLAGS) -MMD -MP -iquote . -c $< -o $@
$(LEPUS_STDLIB_OBJ): $(LEPUS_BUILD_ROOT)/lepus-runtime/buildConfig.json

-include $(wildcard $(EMBEDDED_LSP_BUILD)/*.d)

# BUILD 复用共享原生服务；先构建静态库资产，再嵌入编译器，首次克隆同样自动完成。
LEPUS_BUILD_HOST_BUILD := $(LEPUS_BUILD_ROOT)/lepus-aot-host
LEPUS_BUILD_HOST_OBJECT := $(LEPUS_BUILD_HOST_BUILD)/native_runtime.o
LEPUS_BUILD_SYSTEM_OBJECT := $(LEPUS_BUILD_HOST_BUILD)/system_arena.o
LIB_OBJS += $(LEPUS_BUILD_SYSTEM_OBJECT)
$(LIB): $(LEPUS_BUILD_SYSTEM_OBJECT)
LEPUS_BUILD_PROFILE_OBJECT := $(LEPUS_BUILD_HOST_BUILD)/backend_profile.o
$(LEPUS_BUILD_PROFILE_OBJECT): source/aot/backend_profile.cpp $(LEPUS_BUILD_ROOT)/lepus-runtime/buildConfig.json
	$(PYTHON) -c "from pathlib import Path; Path(r'$(@D)').mkdir(parents=True, exist_ok=True)"
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@
$(LEPUS_BUILD_HOST_OBJECT): source/aot/native_runtime.cpp $(LEPUS_BUILD_ROOT)/lepus-runtime/buildConfig.json
	$(PYTHON) -c "from pathlib import Path; Path(r'$(@D)').mkdir(parents=True, exist_ok=True)"
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@
$(LEPUS_BUILD_SYSTEM_OBJECT): source/allocator/system_arena.cpp $(LEPUS_BUILD_ROOT)/lepus-runtime/buildConfig.json
	$(PYTHON) -c "from pathlib import Path; Path(r'$(@D)').mkdir(parents=True, exist_ok=True)"
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@
-include $(LEPUS_BUILD_HOST_BUILD)/native_runtime.d
-include $(LEPUS_BUILD_HOST_BUILD)/backend_profile.d
-include $(LEPUS_BUILD_HOST_BUILD)/system_arena.d
$(LEPUS_BUILD_HOST_BUILD)/assets.txt: $(LEPUS_BUILD_HOST_OBJECT) $(LEPUS_BUILD_PROFILE_OBJECT) $(LEPUS_BUILD_SYSTEM_OBJECT) $(VM_LIB) $(HOST_LIB) tools/build_aot_host.py tools/embedded_asset_cache.py
	$(PYTHON) tools/build_aot_host.py --build-root "$(LEPUS_BUILD_ROOT)" --cxx "$(CXX)" --ar "$(AR)" --object $(LEPUS_BUILD_HOST_OBJECT) --object $(LEPUS_BUILD_PROFILE_OBJECT) --object $(LEPUS_BUILD_SYSTEM_OBJECT) --output $@

# 更新标准库或胶水实现时只刷新单个内嵌资产对象，不重编宿主入口。
LEPUS_EMBED_INPUTS := $(shell $(PYTHON) tools/embed_stdlib.py --build-root "$(LEPUS_BUILD_ROOT)" --dependencies)
LEPUS_EMBED_STAMP := $(FRONTEND_BUILD)/stdlib_assets.stamp
$(LEPUS_EMBED_STAMP): $(LEPUS_EMBED_INPUTS) | $(FRONTEND_BUILD)
	$(PYTHON) tools/embed_stdlib.py --build-root "$(LEPUS_BUILD_ROOT)" --output "$(LEPUS_EMBED_SOURCE)" --stamp $@
$(LEPUS_EMBED_SOURCE): $(LEPUS_EMBED_STAMP)
	$(PYTHON) tools/embed_stdlib.py --build-root "$(LEPUS_BUILD_ROOT)" --output "$(LEPUS_EMBED_SOURCE)"
$(LEPUS_STDLIB_OBJ): | $(LEPUS_EMBED_STAMP)

# 依赖只落入构建缓存；并行编译须等待校验结束。
.PHONY: lepus-dependencies
lepus-dependencies:
	$(PYTHON) tools/fetch_dependencies.py --dest $(LEPUS_DEPENDENCY_DIR)
$(LIB_OBJS) $(DEMO_OBJS) $(LEPUS_STANDARD_OBJS) $(HOST_BUILD)/ipage_sql.o $(LEPUS_MINIZ): | lepus-dependencies

# 标准库核心二进制装载器；构建生成的字节数组从权威生成脚本维护。
LEPUS_PRECOMPILED_OBJ := $(FRONTEND_BUILD)/lepus_precompiled.o
LIB_OBJS += $(LEPUS_PRECOMPILED_OBJ)
$(LIB): $(LEPUS_PRECOMPILED_OBJ)
$(LEPUS_PRECOMPILED_OBJ): source/lepus_precompiled.cpp source/lepus_precompiled.hpp source/lepus_ast.hpp source/lepus_incremental.hpp source/lepus_package.hpp source/lepus_i18n.hpp $(wildcard source/lepus_precompiled_data.inc) | $(FRONTEND_BUILD)
	$(CXX) $(CXXFLAGS) -MMD -MP -iquote . -c $< -o $@
$(FRONTEND_BUILD)/lepus_main.o: source/lepus_precompiled.hpp
