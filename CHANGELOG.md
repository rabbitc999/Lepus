# Lepus 变更记录

<!-- LANGUAGE NAVIGATION -->
[English（中文原文）](CHANGELOG.md) · **简体中文**
<!-- END LANGUAGE NAVIGATION -->

版本号写在 [`VERSION`](VERSION) 里，格式 `MAJOR.MINOR.PATCH`。当前语言版本为 **1.0.0**：版本使用三段编号，不兼容变更升主版本，兼容功能新增升次版本，修复升补丁版本；所有变更均须记入本文件（改 [`tools/changelog_data.json`](tools/changelog_data.json) 后跑 `--build`），并同步更新 [`doc/21-semantics-frozen.md`](doc/21-semantics-frozen.md) 与 `AGENTS.md`。发行 VS Code 扩展包与编译器使用相同版本；历史开发基线采用三段 0.x.x 编号。较长的开发记录可拆为补丁版本分段，这些分段不重新标记历史二进制、测量结果或既有 tag；其余历史版本保留简要摘要。


## 1.0.0 — Lepus 首个正式版本

- 修复 VS Code 首次打开时的语义着色刷新，完善模块限定宏的着色、补全和悬停，并同步构建目标选择与书稿。
- 扩展 Linux 交叉编译目标至 i686、ARMv7 硬浮点、RV32GC、RV64GC、MIPS32 小端和 MIPS64 小端，同步运行时 SDK、目标汇编、原生对象校验和 VS Code 目标选择。
- 原生 LSP 新增代码块折叠，VSCode 同时支持嵌套花括号与宏指令块折叠。
- 统一使用 LLVM 原生执行。`run` 默认启用运行检查，`build` 默认关闭运行检查并生成发布产物。移除旧解释执行、字节码交付和百分号预处理。
- 完善泛型、闭包、卫生宏与所有权检查，扩展 `for`、枚举载荷自动包装和借用契约。
- 增加宏展开期只读绑定 `$let`、宏外条件编译和构建标志简写 `$!A`。修复嵌套条件下常量作用域与构建标志初始化，支持编译期 f 字符串插值，统一花括号浮点格式的默认精度为八位小数，并更新样例和书稿。第 22 章新增编译期试除法筛选素数的例程。
- 增加支持泛型的透明类型别名，标准库 `Result` 的失败变体命名为 `Fail`。`std::core` 提供 `StdResult<T>`。LSP 支持跨模块别名跳转、展开预览和限定枚举变体补全。
- 核心标准库统一为自动加载的 `std::core`。新增 GUI、音频、XML 和密码学模块，完善统一 C ABI 与同步回调。
- 完善 VS Code 语言服务和 GDB 原生调试，同步双语教程、中文书稿与离线手册。LSP 并行前处理独立导入，在小改动造成语法错误时恢复未受影响的类型提示，并定期清理非当天缓存。 插值表达式内支持语义着色和精确范围的定义链接。修复移动匹配的载荷转移，支持递归 Box 遍历与 Rc、Arc 等拥有句柄的安全解构。
- 发布 Windows x64 静态和外置 LLVM 构件、MSI 与 VSIX，以及 Ubuntu 22.04 x64 和 CentOS Stream 9 x64 安装包。Ubuntu 22.04 ARM64 包提供前端和 LSP。
