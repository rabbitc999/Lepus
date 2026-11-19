/**
 * @file aot_bind_module.h
 * @brief 在模块所属编译单元中生成唯一的 BUILD 注册入口。
 */
#include "lepus_standard_internal.h"
#ifndef LEPUS_BUILD_BIND_MODULE
#error "LEPUS_BUILD_BIND_MODULE must name the owning module"
#endif

/**
 * @brief 提供模块局部的 C 表登记实现，未使用的静态内联实现不产生链接引用。
 * @param target 未封存的注册句柄。
 * @param failure 接收诊断的有界缓冲区。
 * @return C ABI 状态码。
 */
#define LEPUS_BUILD_MODULE(name, ...)                                                                                       \
    static inline int32_t lepus_local_bind_##name(LepusAbiRegistry *target, LepusAbiError *failure)                        \
    {                                                                                                                     \
        const LepusAbiModule modules[] = { __VA_ARGS__ };                                                                  \
        return lepus_abi_register_tables(target, modules, sizeof(modules) / sizeof(modules[0]), failure);                   \
    }
#include "aot_modules.inc"
#undef LEPUS_BUILD_MODULE
#define LEPUS_BUILD_JOIN_DETAIL(left, right) left##right
#define LEPUS_BUILD_JOIN(left, right) LEPUS_BUILD_JOIN_DETAIL(left, right)
#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief 仅登记当前模块，应用链接不引用编译器的全量选择表。
 * @param target 未封存的注册句柄。
 * @param failure 接收诊断的有界缓冲区。
 * @return C ABI 状态码。
 */
int32_t LEPUS_BUILD_JOIN(lepus_std_bind_, LEPUS_BUILD_BIND_MODULE)(LepusAbiRegistry *target, LepusAbiError *failure)
{
    return LEPUS_BUILD_JOIN(lepus_local_bind_, LEPUS_BUILD_BIND_MODULE)(target, failure);
}
#ifdef __cplusplus
}
#endif
#undef LEPUS_BUILD_JOIN
#undef LEPUS_BUILD_JOIN_DETAIL
#undef LEPUS_BUILD_BIND_MODULE
