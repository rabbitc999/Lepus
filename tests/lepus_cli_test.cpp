/**
 * @file lepus_cli_test.cpp
 * @brief 场景参数归一化单元测试，覆盖值保护及拖入文件参数边界。
 */
#include "../source/lepus_cli.hpp"
#include <cassert>
#include <initializer_list>

/**
 * @brief 检查一组输入参数的完整转换结果。
 * @param input 输入。
 * @param expected 期望输出。
 */
static void check(std::initializer_list<const char *> input, std::initializer_list<const char *> expected)
{
    std::vector<std::string> storage(input.begin(), input.end());
    std::vector<char *> args;
    for (auto &value : storage) args.push_back(value.data());
    assert(lepus_cli::normalize(static_cast<int>(args.size()), args.data()) == std::vector<std::string>(expected.begin(), expected.end()));
}

/**
 * @brief 运行场景命令与参数保护用例。
 * @return 全部断言通过后返回 0。
 */
int main()
{
    check({ "lepus", "demo.lep", "out", "--check", "two words" }, { "lepus", "run", "demo.lep", "--", "out", "--check", "two words" });
    check({ "lepus", "demo.lep", "--", "debug" }, { "lepus", "run", "demo.lep", "--", "debug" });
    check({ "lepus", "check", "demo.lep", "json" }, { "lepus", "--check", "demo.lep", "--diagnostics=json" });
    check({ "lepus", "new", "demo", "name", "help" }, { "lepus", "new", "demo", "--name", "help" });
    check({ "lepus", "new", "name" }, { "lepus", "new", "name" });
    check({ "lepus", "emit", "demo.lep", "out", "debug", "reflect" },
          { "lepus", "emit", "demo.lep", "--out", "debug", "--reflection" });
    check({ "lepus", "run", "out", "--out", "help", "--", "reflect" },
          { "lepus", "run", "out", "--out", "help", "--", "reflect" });
    check({ "lepus", "test", "tests", "backend", "interpret", "json" }, { "lepus", "test", "tests", "--backend", "interpret", "--json" });
    check({ "lepus", "fmt", "check", "src" }, { "lepus", "fmt", "--check", "src" });
    check({ "lepus", "deps", "lock", "project", "out", "offline" }, { "lepus", "deps", "lock", "--project", "out", "--offline" });
    check({ "lepus", "gen", "compiler", "out", "tmp" }, { "lepus", "gen", "compiler", "--out-dir", "tmp" });
    check({ "lepus", "clear", "out", "tmp" }, { "lepus", "clear", "--out-dir", "tmp" });
    check({ "lepus", "help" }, { "lepus", "--help" });
    check({ "lepus", "build", "demo.lep", "out", "help", "--emit-obj" }, { "lepus", "build", "demo.lep", "--out", "help", "--emit-obj" });
    check({ "lepus", "build", "--target", "linux-arm64", "demo.lep", "out", "native" }, { "lepus", "build", "--target", "linux-arm64", "demo.lep", "--out", "native" });
    check({ "lepus", "build", "demo.lep", "--", "out", "help" }, { "lepus", "build", "demo.lep", "--", "out", "help" });
    check({ "lepus", "--check", "demo.lep", "--out", "help" }, { "lepus", "--check", "demo.lep", "--out", "help" });
    check({ "lepus", "run", "-B", "out", "demo.lep" }, { "lepus", "run", "-B", "out", "demo.lep" });
    check({ "lepus", "build", "demo.lep", "-B", "debug" }, { "lepus", "build", "demo.lep", "-B", "debug" });
    check({ "lepus", "run", "-BA", "-BB", "demo.lep" }, { "lepus", "run", "-BA", "-BB", "demo.lep" });
    check({ "lepus", "run", "-I", "debug", "-B", "out", "demo.lep" }, { "lepus", "run", "-I", "debug", "-B", "out", "demo.lep" });
    check({ "lepus", "run", "demo.lep", "--", "-B", "out" }, { "lepus", "run", "demo.lep", "--", "-B", "out" });
}
