#!/bin/sh
# lepus/ci.sh —— Lepus 的门禁：分两档。
#
# 用法（在本目录跑）：
#   sh ci.sh                 快速门禁（默认）：构建 + 样例回归 + 核心测试 + 文档门禁
#   sh ci.sh --jobs 4       并行独立回归；--jobs 1 串行（也可设 LEPUS_GATE_JOBS）
#   sh ci.sh --help
#
# 环境要求：g++ / gcc / make / python3.11+ 必需；cmake、node、npm 仅发布门禁需要。
# 发布门禁的真实扩展测试还需 VSCode、GDB 和图形会话；可用 LEPUS_VSCODE_EXECUTABLE 指定宿主。
# 还要一份 **ParseEngine 文法生成器**：先查 ENGINE、仓库根目录及相邻 ParseEngine 构建目录，
# 它由 ParseEngine 仓库（不含在本项目里）产出；不在就报错退出 2。
#
# 退出码：0 全绿；1 有检查失败；2 环境不满足。

set -u
# 诊断与样例输出都是中文，显示语言固定，免得随执行主机变化。
export LEPUS_LANG=zh
# 业务门禁只比较程序输出；完成提示由独立专项清除静默后核对。
export LEPUS_QUIET=1
export PARSEENGINE_LANG=zh
export PYTHONUTF8=1
export PYTHONIOENCODING=utf-8

root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 2
cd "$root" || exit 2

run_python=1
full=0
jobs=${LEPUS_GATE_JOBS:-4}
while [ "$#" -gt 0 ]; do
    arg=$1
    shift
    case "$arg" in
    --jobs)
        [ "$#" -gt 0 ] || { echo "--jobs 需要正整数" >&2; exit 2; }
        jobs=$1
        shift
        ;;
    --jobs=*) jobs=${arg#--jobs=} ;;
    --no-python) run_python=0 ;;
    --full) full=1 ;;
    -h | --help)
        awk 'NR > 1 && /^#/ { sub(/^# ?/, ""); print; next } NR > 1 { exit }' "$0"
        exit 0
        ;;
    *)
        echo "未知参数：$arg（用 --help 看用法）" >&2
        exit 2
        ;;
    esac
done

case "$jobs" in ''|*[!0-9]*|0) echo "--jobs 需要正整数" >&2; exit 2 ;; esac
[ "$jobs" -gt 0 ] 2>/dev/null || { echo "--jobs 需要正整数" >&2; exit 2; }

have() { command -v "$1" >/dev/null 2>&1; }

echo "== 环境检测 =="
missing=''
# make：优先 GNU make，Windows（MSYS）环境退回 mingw32-make。
MAKE=${MAKE:-}
if [ -z "$MAKE" ]; then
    if have make; then
        MAKE=make
    elif have mingw32-make; then
        MAKE=mingw32-make
    fi
fi
if [ -n "$MAKE" ] && have "$MAKE"; then
    printf '  %-8s %s\n' make "$(command -v "$MAKE")"
else
    printf '  %-8s 缺失（必需）\n' make
    missing="$missing make"
fi
for tool in g++ gcc; do
    if have "$tool"; then
        printf '  %-8s %s\n' "$tool" "$(command -v "$tool")"
    else
        printf '  %-8s 缺失（必需）\n' "$tool"
        missing="$missing $tool"
    fi
done
# 完整发布门禁还构建原生测试并执行编辑器、调试及 VSIX 打包验收。
if [ "$full" = 1 ]; then
    for tool in cmake node npm; do
        if have "$tool"; then
            printf '  %-8s %s\n' "$tool" "$(command -v "$tool")"
        else
            printf '  %-8s 缺失（必需）\n' "$tool"
            missing="$missing $tool"
        fi
    done
fi
PYTHON=''
for candidate in python3 python; do
    if have "$candidate" && [ "$("$candidate" -c 'import sys; print("LEPUS-PYTHON-OK" if sys.version_info >= (3, 11) else "too-old")' 2>/dev/null)" = "LEPUS-PYTHON-OK" ]; then PYTHON="$candidate"; break; fi
done
if [ -n "$PYTHON" ]; then
    mkdir -p "$root/tmp"
    LEPUS_PROJECT_TMP=$($PYTHON -c "from pathlib import Path; print(Path('tmp').resolve().as_posix())")
    export TMPDIR="$LEPUS_PROJECT_TMP" TMP="$LEPUS_PROJECT_TMP" TEMP="$LEPUS_PROJECT_TMP"
    printf '  %-8s %s\n' python "$(command -v "$PYTHON")"
else
    printf '  %-8s 缺失（必需）\n' python
    missing="$missing python"
fi
# 文法生成器是外部依赖：本项目只有 .gram，生成器由 ParseEngine 仓库产出。
# 查找顺序与 gen.sh 保持一致——先看本项目根目录，再退回 ParseEngine 仓库那两处
# （本仓是独立工程，生成器就放在根目录；只认 ../build/bin/ 会漏掉它）。
# Windows 上 `.exe` 排在前面：MSYS 的 `test -x ParseEngine` 会因 .exe 后缀假命中。
if [ -n "${ENGINE:-}" ]; then
    engine_found="$ENGINE"
else
    engine_found=''
    for candidate in "./ParseEngine.exe" "./ParseEngine" \
                     "../ParseEngine/build/bin/ParseEngine.exe" "../ParseEngine/build/bin/ParseEngine" \
                     "../build/bin/ParseEngine.exe" "../build/bin/ParseEngine"; do
        if [ -x "$candidate" ]; then engine_found="$candidate"; break; fi
    done
fi
if [ -n "$engine_found" ] && [ -x "$engine_found" ]; then
    printf '  %-8s %s\n' engine "$engine_found"
    ENGINE="$engine_found"
else
    printf '  %-8s 缺失（必需）：用 ENGINE=<路径> 指一份 ParseEngine 生成器\n' engine
    missing="$missing ParseEngine"
fi
if [ -n "$missing" ]; then
    echo
    echo "缺少必需工具：$missing —— 装好再跑。" >&2
    exit 2
fi

# 先登记完整命令；Python 调度器仅并行明确审查过的独立 Python 与原生回归。
# 生成、构建和未知任务形成屏障，确保不会边运行编译器边覆盖可执行文件。
export ENGINE
LEPUS_BIN=$($PYTHON tools/compiler_path.py) || exit 2
export LEPUS_BIN
LEPUS_COMPILER=$LEPUS_BIN
DEMO=$LEPUS_BIN
export LEPUS_COMPILER DEMO
mkdir -p tmp/ci/logs
manifest=$(mktemp tmp/ci/gates.XXXXXX) || exit 2
trap 'rm -f "$manifest"' EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
run_step() {
    "$PYTHON" tools/parallel_gates.py append "$manifest" "$@" || exit 2
}
run_required_step() {
    "$PYTHON" tools/parallel_gates.py append --required "$manifest" "$@" || exit 2
}


# ---- 3. Lepus：由文法生成解析器，再编驱动 ----
if [ "$full" = 1 ] && [ "${OS:-}" = Windows_NT ]; then
    run_required_step "校验并恢复 LLVM 离线工具链" "$PYTHON" library/llvm/setup.py
    run_step "检查 BUILD 运行时 LLVM 模块同步" sh tools/build_aot_runtime.sh --check
fi
run_required_step "生成 LEPUS 解析器（gen.sh）" env LEPUS_DEFER_ASSET_EMBED=1 sh gen.sh
run_required_step "构建 LEPUS 驱动（lepus）" "$MAKE" -j4

run_step "检查命令、项目配置与旧运行时文案" "$PYTHON" tests/lepus_cli_contract.py
run_step "验证 build 发布命令与拒绝旧入口" "$PYTHON" tests/lepus_build_command.py --lepus "$LEPUS_BIN"
run_step "检测文档过时表述" "$PYTHON" tests/lepus_doc_history.py
run_step "检查更改记录同步" "$PYTHON" tools/changelog.py --check
run_step "检查性能报告计算与完整性" "$PYTHON" tests/lepus_benchmark_report.py
run_step "验证并行门禁调度与失败汇总" "$PYTHON" tests/test_parallel_gates.py
run_step "验证大整数乘除法与 Python 整数差分" "$PYTHON" tests/bigint_arithmetic.py
run_step "验证已确认的编译器安全边界" "$PYTHON" tests/lepus_compiler_safety.py
run_step "验证高精度数学与独立参考值" "$PYTHON" tests/bigfloat_math.py
run_step "验证 CAS 与泛型数学语言接口" "$PYTHON" tests/lepus_cas_math.py
run_step "验证 opcode 值语义、红黑删除与字符串方法" "$PYTHON" tests/lepus_opcode_values.py
run_step "验证动态反射入口已移除" "$PYTHON" tests/lepus_reflection_calls.py --lepus "$LEPUS_BIN"
run_step "验证反射序列化与注解动态 SQL" "$PYTHON" tests/lepus_reflection_serialization.py
run_step "验证 CAS 核心全部回归" "$PYTHON" tests/cas_library.py
run_step "验证卫生宏绑定、正则、AST 替换及生成解析器" "$PYTHON" tests/macro_foundations.py
run_step "验证 AST 表达式宏语义与原生执行" "$PYTHON" tests/lepus_ast_macros.py --default
run_step "验证卫生宏参数副本、引用与单次求值" "$PYTHON" tests/lepus_macro_parameters.py --lepus "$LEPUS_BIN"
run_step "验证接口值、联合匹配与传播转换" "$PYTHON" tests/lepus_interface_values.py --lepus "$LEPUS_BIN"
run_step "验证 else if 与 elif 统一分支语义" "$PYTHON" tests/lepus_else_if.py --lepus "$LEPUS_BIN"
run_step "验证宏完整片段分类与正则分支" "$PYTHON" tests/lepus_macro_branches.py --default
run_step "验证卫生宏参数分组、片段循环与调用来源" "$PYTHON" tests/lepus_macro_groups.py --default
run_step "验证卫生宏静态类型参数推导" "$PYTHON" tests/lepus_macro_generics.py --default
run_step "验证宏显式结果来源与混合类型推导" "$PYTHON" tests/lepus_macro_result_inference.py --lepus "$LEPUS_BIN"
run_step "验证宏显式参数类别与求值语义" "$PYTHON" tests/lepus_macro_categories.py --lepus "$LEPUS_BIN"

# ---- 快速门禁核心三件套：样例回归 + 本次新增能力测试 + 文档同步 ----
run_step "LEPUS 样例回归（check.sh）" \
    sh -c 'cd "$1" && LEPUS_TMP="$1/tmp/ci/check" sh check.sh' sh "$root"
run_step "运行 std::image 与 StringBuilder 回归" "$PYTHON" tests/lepus_image.py --lepus "$LEPUS_BIN"
run_step "运行标准错误流与文件打印回归" "$PYTHON" tests/lepus_print_streams.py --lepus "$LEPUS_BIN"
run_step "运行线程超时与非阻塞等待回归" "$PYTHON" tests/lepus_thread_wait.py --lepus "$LEPUS_BIN"
run_step "运行临时值与资源释放合并回归" "$PYTHON" tests/lepus_resource_lifetime.py --lepus "$LEPUS_BIN"
run_step "运行闭包自动类型与嵌套返回语言服务回归" "$PYTHON" tests/lepus_lsp_closure_types.py --lepus "$LEPUS_BIN"
run_required_step "编译无墓碑分配身份回归" g++ -std=c++17 -O2 -DLEPUS_CUSTOM_ALLOCATOR=1 -Isource -Ilibrary -Isource/allocator tests/allocation_identity.cpp -o tmp/ci/allocation-identity
run_step "运行百万次分配身份回归" tmp/ci/allocation-identity
run_required_step "编译地址视图与调试内存目录回归" g++ -std=c++17 -O2 -DLEPUS_CUSTOM_ALLOCATOR=1 -Isource -Ilibrary -Isource/allocator tests/lepus_heap_address_view.cpp source/ircode/runtime_heap.cpp source/allocator/system_arena.cpp -o tmp/ci/heap-address-view
run_step "运行地址视图与调试内存目录回归" tmp/ci/heap-address-view
run_step "运行 C ABI 契约与直通胶水入口表回归" "$PYTHON" tests/lepus_c_api_contract.py
run_step "运行数组按值传参与引用回归" "$PYTHON" tests/lepus_array_parameters.py --lepus "$LEPUS_BIN"
run_required_step "编译字面量数据合并单元测试" g++ -std=c++17 -O2 -DLEPUS_CUSTOM_ALLOCATOR=0 -DLEPUS_WITH_REFLECTION=1 -DRUNTIME_WITH_REFLECTION=1 source/ast/test_ast.cpp source/ast/ast.cpp source/ast/ast_type.cpp source/ast/ast_node.cpp source/ast/ast_data.cpp -o tmp/ci/simple-ast-literals
run_step "运行字面量数据合并单元测试" tmp/ci/simple-ast-literals
run_step "检查诊断摘录与 TOML 注释往返" "$PYTHON" tests/lepus_diagnostic_layout.py --lepus "$LEPUS_BIN"
run_step "运行字面量合并与字符串副本回归" "$PYTHON" tests/lepus_string_literals.py --lepus "$LEPUS_BIN"
run_step "检查 opcode 声明与不可复制限制" "$PYTHON" tests/lepus_opcode_declarations.py --lepus "$LEPUS_BIN"
run_step "检查区间与循环值表达式" "$PYTHON" tests/lepus_range_loop_values.py --lepus "$LEPUS_BIN"
run_step "检查 opcode 资源所有权与借用" "$PYTHON" tests/lepus_opcode_resources.py --lepus "$LEPUS_BIN"
run_step "检查 opcode 反射与语言服务隐藏" "$PYTHON" tests/lepus_opcode_visibility.py --lepus "$LEPUS_BIN"
run_step "检查 BUILD 共享运行时构建与资产清单同步" "$PYTHON" tools/stackvm_sources.py --check
run_required_step "编译 TOML 注释保存回归" g++ -std=c++17 -O2 source/toml/test_toml.cpp source/toml/toml.cpp source/toml/toml_source.cpp -o tmp/ci/toml-comments
run_step "运行 TOML 注释保存回归" tmp/ci/toml-comments
run_step "检查标准对象与运行期核心隔离" "$PYTHON" tests/opcode_core_isolation.py
run_step "检查 BUILD 集合传参、返回值与复制语义" "$PYTHON" tests/lepus_collection_glue.py --lepus "$LEPUS_BIN"
run_step "检查 Map Pair 迭代元素类型" "$PYTHON" tests/lepus_map_pair_iterator.py --lepus "$LEPUS_BIN"
run_step "检查泛型命名构造入口推导" "$PYTHON" tests/lepus_generic_constructors.py --lepus "$LEPUS_BIN"
run_step "检查标准库 opcode 存储与资源边界" "$PYTHON" tests/lepus_opcode_std_storage.py --lepus "$LEPUS_BIN"
run_step "运行闭包来源与借用控制流回归" "$PYTHON" tests/lepus_borrow_flow.py --lepus "$LEPUS_BIN"
run_step "运行引用移动、延迟清理与原生错误位置回归" "$PYTHON" tests/lepus_borrow_move_defer.py --lepus "$LEPUS_BIN"
run_step "运行显式借用契约边界与压力回归" "$PYTHON" tests/lepus_borrow_contract_stress.py --lepus "$LEPUS_BIN"
run_step "运行嵌套 Result 与错误传播压力回归" "$PYTHON" tests/lepus_nested_result_stress.py --lepus "$LEPUS_BIN"
run_step "运行复杂语法、格式化往返与语义组合压力" "$PYTHON" tests/lepus_syntax_semantic_stress.py --lepus "$LEPUS_BIN"
run_step "运行目标宏与布尔构建选项样例" "$PYTHON" tests/lepus_build_options.py --lepus "$LEPUS_BIN"
run_step "运行泛型闭包与特化回归" "$PYTHON" tests/lepus_generic_closures.py --lepus "$LEPUS_BIN"
run_step "检查模块路径与文本包含指令删除" "$PYTHON" tests/lepus_package_paths.py
run_step "检查统一入口、产物布局与未知文件" "$PYTHON" tests/lepus_tool.py
run_step "检查手册与标准库共享索引同步" "$PYTHON" tools/library_reference.py --check
run_step "检查标准库命名与内部函数可见性" "$PYTHON" tools/check_std_naming.py
