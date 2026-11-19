/** @file lepus_standard_internal.h @brief C 模块取表及登记的共享实现。 */
#ifndef LEPUS_STANDARD_INTERNAL_H
#define LEPUS_STANDARD_INTERNAL_H
#include "lepus_standard.h"
#include "lepus_abi_internal.h"

/**
 * @brief 按声明顺序登记模块入口表，在首个失败处停止。
 * @param registry 未封存的 C 注册句柄。
 * @param modules 非空的模块取表函数数组。
 * @param count 数组元素数量。
 * @param error 可选的调用方诊断缓冲。
 * @return 稳定状态码；每张入口表的登记各自具有原子性。
 */
static inline int32_t lepus_abi_register_tables(LepusAbiRegistry *registry, const LepusAbiModule *modules,
                                                size_t count, LepusAbiError *error)
{
    size_t index;
    if (!registry || !modules || !count)
        return lepus_abi_status(error, LepusAbiInvalidArgument, "missing registry or module tables");
    for (index = 0; index < count; ++index)
    {
        size_t length = 0;
        const LepusAbiRecord *records;
        LepusAbiInfo info;
        LepusAbiModuleDescriptor descriptor;
        int32_t status;
        if (!modules[index])
            return lepus_abi_status(error, LepusAbiInvalidArgument, "missing module table provider");
        records = modules[index](&length);
        info = lepus_abi_local_info(0);
        descriptor.structSize = sizeof(descriptor);
        descriptor.abi = &info;
        descriptor.records = records;
        descriptor.count = length;
        status = lepus_abi_registry_register(registry, &descriptor, error);
        if (status != LepusAbiOk) return status;
    }
    return lepus_abi_status(error, LepusAbiOk, 0);
}
#endif
