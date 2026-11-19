# 01 五分钟快速开始

<!-- LANGUAGE NAVIGATION -->
[English](en/01-quickstart.md) · **简体中文**
<!-- END LANGUAGE NAVIGATION -->

目标是让你在五分钟内跑起第一个 Lepus 程序。不需要先理解值语义和引用，那些留给[核心概念](03-core-concepts.md)。

---

## 1. 安装

两条路，选一条：

**方式一：用发布包**

**方式二：从源码构建**

```sh
sh gen.sh      # 用 IELR 从 lepus.gram 生成前端（需要 HareParse 文法生成器）
make           # 构建 lepus 与 build/lepus-frontend/libparser.a
```

`gen.sh` 需要一份 [HareParse](https://github.com/rabbitc999/HareParse) 文法生成器。
最省事是把它放到项目根目录 `./HareParse`（Windows 加 `.exe`）；也可以让它待在旁边的
HareParse 仓库里，`gen.sh` 会自动去找 `../HareParse/build/bin/`。

Windows 上未找到生成器时，`gen.sh` 按尝试  [GitHub](https://github.com/rabbitc999/HareParse/releases/download/latest/HareParse.exe) 两处的最新发布版，先成功的那个下载为项目根目录的 `HareParse.exe`；已有生成器会直接复用。其他平台请提供可运行的本机生成器，或通过 `ENGINE` 指定路径。

验证装好了：

```sh
lepus help
```

> 命令行中 `lepus` 在 Windows 上是 `Lepus.exe`。Git Bash / MSYS 下写 `./lepus` 能自动找到 `Lepus.exe`，
> 光写 `lepus` 会被当成 PATH 查找。

---

## 2. 第一个程序

新建 `hello.lep`：

```lepus
void main() {
    println("Hello, Lepus!");
}
```

三件事值得注意：

- 入口函数固定叫 `main`，返回 `void`；`void main` 正常结束返回进程退出码 0。
- `println` 来自自动加载的核心库，完整写法是 `std::core::println`；没有命名冲突时前缀可省。
- 分号和大括号的用法和 C 系语言一样 —— 这是刻意的，Lepus 借用 C 的表达式与块语法。

---

## 3. 运行

```sh
lepus run hello.lep
```

输出：

```text
Hello, Lepus!
```

把源码文件直接交给 `lepus` 也会使用 checked 原生构建 编译并运行，后面的内容全部作为程序参数传给 `main`：

```sh
lepus hello.lep Alice "two words"
```

需要参数时把入口写成：

```lepus
void main(List<String> arguments) {
    println("hello, {}", arguments[0]);
}
```

---

## 4. 发生了什么

`lepus run` 完成以下四步，成功编译后启动生成的原生程序：

1. **加载源码** —— 加载 `import` 模块，维护来源位置，展开插值和宏；
2. **解析与类型检查** —— 语法、类型、引用生命周期检查都在这里，不通过就不执行；
3. **生成 LLVM IR** —— 正常运行路径从已检查的 AST 直接降到 LLVM IR；Lepus 四元式是单独的导出路径；
4. **LLVM 构建 编译执行** —— LLVM 生成带运行时检查的原生程序，发布到源码同级、提示编译完成，然后启动程序。

想只看前两步（不执行、不产出文件）：

```sh
lepus check hello.lep
```

---

## 5. 下一步

| 你想 | 去哪 |
| --- | --- |
| 了解语言特性 | [语言速览](02-language-tour.md) |
| 把程序部署到目标宿主 | [交付安全](05-delivery-security.md) → [跨平台部署](06-deployment.md) |
| 看看标准库有什么 | [标准库概览](07-stdlib-overview.md) |
| 建一个正式工程 | `lepus new my-app`，配置见[工具链使用](09-toolchain.md) |

---

## 6. 常用命令速查

| 你想做的事 | 命令 |
| --- | --- |
| 编译并运行 | `lepus run src/main.lep` |
| 只检查不运行 | `lepus check src/main.lep` |
| 输出 JSON 诊断（给编辑器用） | `lepus check src/main.lep json` |
| 导出可执行文件 | `lepus build src/main.lep --out build/app` |
| 导出 LLVM IR | `lepus build src/main.lep --emit-llvm --out build/app` |
| 新建工程 | `lepus new hello` |
| 跑测试 | `lepus test tests` |
| 格式检查 / 写回 | `lepus fmt check src` / `lepus fmt write src` |
| 查看帮助 | `lepus help` |

更多子命令、`lepus.toml` 配置与编译选项见[工具链使用](09-toolchain.md)。

解析器生成源码、头文件与报告统一位于 `source/`；对象文件与解析器静态库位于 `build/lepus-frontend/`，不会散落到仓库根目录。
