/** @file c_api_library_host.c @brief C 函数表调用原生语言库，闭包回调及资源释放由真实程序验证。 */
#include "lepus_abi.h"
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
typedef HMODULE Library;
#else
#include <dlfcn.h>
typedef void *Library;
#endif
typedef void (LEPUS_ABI_CALL *Lifecycle)(void);
typedef int64_t (LEPUS_ABI_CALL *Calculate)(int64_t);
typedef int64_t (LEPUS_ABI_CALL *Count)(void);
static Calculate calculate = NULL;
/**
 * @brief 装载真实 C 动态库。
 * @param path 库路径。
 * @return 库句柄或空值。
 */
static Library open_library(const char *path)
{
#ifdef _WIN32
    return LoadLibraryA(path);
#else
    return dlopen(path, RTLD_NOW | RTLD_LOCAL);
#endif
}
/**
 * @brief 查找真实平台 C 符号，不涉及语言函数句柄强转。
 * @param library 库句柄。
 * @param name C 导出名。
 * @return 符号地址或空值。
 */
static void *symbol(Library library, const char *name)
{
#ifdef _WIN32
    return (void *)GetProcAddress(library, name);
#else
    return dlsym(library, name);
#endif
}
/**
 * @brief 释放已结束使用的库引用。
 * @param library 库句柄。
 */
static void close_library(Library library)
{
#ifdef _WIN32
    FreeLibrary(library);
#else
    dlclose(library);
#endif
}
/**
 * @brief 从 C 帧读取整数，再调用原生库的真实 C 导出。
 * @param frame 本次调用帧。
 */
static void LEPUS_ABI_CALL forward(LepusAbiFrame *frame)
{
    int64_t value = 0;
    if (!lepus_abi_argument_integer(frame, 0, &value)) return;
    lepus_abi_return_integer(frame, calculate(value));
}
/**
 * @brief 装载两份库，登记并执行，核对释放后完成清理。
 * @param argc 参数数量。
 * @param argv 语言库和 C 库路径。
 * @return 输出为 42 且返回文本恰好释放一次时为零。
 */
int main(int argc, char **argv)
{
    if (argc != 3) return 2;
    Library provider = open_library(argv[2]);
    Library language = open_library(argv[1]);
    if (!provider || !language) return 3;
    Lifecycle initialize = (Lifecycle)symbol(language, "lepus_init");
    Lifecycle finish = (Lifecycle)symbol(language, "lepus_finish");
    Count releases = (Count)symbol(provider, "release_count");
    calculate = (Calculate)symbol(language, "calculate");
    if (!initialize || !finish || !releases || !calculate) return 4;
    initialize();
    char message[256];
    LepusAbiError error = {message, sizeof(message), 0, 0};
    LepusAbiRegistry *registry = lepus_abi_registry_create(&error);
    LepusAbiInfo abi = lepus_abi_local_info(LEPUS_ABI_CAP_REGISTRY);
    LepusAbiRecord record = {"app::calculate", forward, 1};
    LepusAbiModuleDescriptor module = {sizeof(module), &abi, &record, 1};
    int64_t value = 40;
    unsigned char kind = LepusAbiSlotInteger;
    LepusAbiFrame frame = {NULL, &value, &kind, 1, 0, 0};
    int failed = !registry || lepus_abi_registry_register(registry, &module, &error) ||
                 lepus_abi_registry_seal(registry, &error) ||
                 lepus_abi_registry_call(registry, "app::calculate", &frame, &error) ||
                 frame.result != 42 || releases() != 1;
    if (failed) fprintf(stderr, "C native host: %s\n", message);
    finish(); /* 函数表不得借用已经销毁的语言 arena。 */
    lepus_abi_registry_destroy(registry);
    close_library(language);
    close_library(provider);
    if (!failed) puts("C registry -> native language -> captured C callback: 42, owned result released once");
    return failed;
}
