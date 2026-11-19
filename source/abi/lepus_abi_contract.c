/**
 * @file lepus_abi_contract.c
 * @brief 无分配的 C ABI 目标、模块和调用帧契约检查。
 */
#include "lepus_abi_internal.h"

#include <string.h>

_Static_assert(sizeof(double) == sizeof(int64_t), "C ABI requires 64-bit double slots");

/**
 * @brief 写入有界诊断并返回状态；首次或复用缓冲均可。
 * @param error 调用方缓冲，可为空。
 * @param status 本次状态。
 * @param message 失败诊断；成功时为空。
 * @return 输入状态。
 */
int lepus_abi_status(LepusAbiError *error, int status, const char *message)
{
    if (error != NULL)
    {
        const size_t length = message == NULL ? 0u : strlen(message);
        error->status = status;
        error->required = status == LepusAbiOk ? 0u : length + 1u;
        if (error->data != NULL && error->capacity != 0u)
        {
            const size_t count = length < error->capacity ? length : error->capacity - 1u;
            if (count != 0u) memcpy(error->data, message, count);
            error->data[count] = '\0';
        }
    }
    return status;
}

/**
 * @brief 返回静态只读的宿主布局；查询不写共享数据。
 * @return 当前目标描述的进程级地址。
 */
const LepusAbiInfo *lepus_abi_info(void)
{
    /* 按常量初始化，避免并发首次查询时写共享存储。 */
    static const LepusAbiInfo info = {
        sizeof(LepusAbiInfo), LEPUS_ABI_VERSION, sizeof(void *) * 8u,
#if defined(_WIN32) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
        1,
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        0,
#else
#error "请为此目标定义字节序后构建 C ABI"
#endif
        LEPUS_ABI_TARGET_CONVENTION,
        LEPUS_ABI_TARGET_ARCH,
        LEPUS_ABI_TARGET_PLATFORM,
        sizeof(LepusAbiFrame),
        _Alignof(LepusAbiFrame),
        sizeof(LepusAbiRecord),
        _Alignof(LepusAbiRecord),
        { offsetof(LepusAbiFrame, runtime), offsetof(LepusAbiFrame, slots), offsetof(LepusAbiFrame, kinds), offsetof(LepusAbiFrame, count), offsetof(LepusAbiFrame, result), offsetof(LepusAbiFrame, resultIsFloat) },
        { offsetof(LepusAbiRecord, name), offsetof(LepusAbiRecord, entry), offsetof(LepusAbiRecord, arity) },
        LEPUS_ABI_CAP_SYNC_CALLBACK | LEPUS_ABI_CAP_RECORD | LEPUS_ABI_CAP_BYTE_VIEW | LEPUS_ABI_CAP_REGISTRY | LEPUS_ABI_CAP_MEMORY | LEPUS_ABI_CAP_BINARY | LEPUS_ABI_CAP_RECORD_CLONE | LEPUS_ABI_CAP_RECORD_PROBE | LEPUS_ABI_CAP_LIST_TEXT | LEPUS_ABI_CAP_LIST_NUMBERS | LEPUS_ABI_CAP_RECORD_TEXT | LEPUS_ABI_CAP_INPUT_TOKEN | LEPUS_ABI_CAP_TRACKED_TEMPORARY
    };
    return &info;
}

/**
 * @brief 逐字段比较模块与宿主布局及所需能力。
 * @param expected 模块本地生成的布局。
 * @param error 调用方有界诊断缓冲，可为空。
 * @return 检查状态，零表示兼容。
 */
int lepus_abi_check(const LepusAbiInfo *expected, LepusAbiError *error)
{
    const LepusAbiInfo *host = lepus_abi_info();
    if (expected == NULL) return lepus_abi_status(error, LepusAbiInvalidArgument, "missing ABI descriptor");
    /* 先核对大小；短描述不能读取后续字段。 */
    if (expected->structSize != sizeof(LepusAbiInfo))
        return lepus_abi_status(error, LepusAbiIncompatible, "ABI descriptor size mismatch");
    if (expected->version != host->version)
        return lepus_abi_status(error, LepusAbiIncompatible, "ABI version mismatch");
    if (host->architecture == 0 || host->platform == 0 || expected->architecture != host->architecture || expected->platform != host->platform)
        return lepus_abi_status(error, LepusAbiIncompatible, "ABI architecture or platform mismatch");
    if (expected->pointerBits != host->pointerBits || expected->littleEndian != host->littleEndian)
        return lepus_abi_status(error, LepusAbiIncompatible, "ABI target layout mismatch");
    if (expected->callingConvention != host->callingConvention)
        return lepus_abi_status(error, LepusAbiIncompatible, "ABI calling convention mismatch");
    if (expected->frameSize != host->frameSize || expected->frameAlignment != host->frameAlignment || expected->recordSize != host->recordSize || expected->recordAlignment != host->recordAlignment || memcmp(expected->frameOffsets, host->frameOffsets, sizeof(host->frameOffsets)) != 0 || memcmp(expected->recordOffsets, host->recordOffsets, sizeof(host->recordOffsets)) != 0)
        return lepus_abi_status(error, LepusAbiIncompatible, "ABI frame or record layout mismatch");
    if ((expected->capabilities & ~host->capabilities) != 0u)
        return lepus_abi_status(error, LepusAbiIncompatible, "unsupported ABI capability");
    return lepus_abi_status(error, LepusAbiOk, NULL);
}

/**
 * @brief 无副作用检查模块全部入口及模块内重名。
 * @param module 调用方提供的模块描述。
 * @param error 调用方有界诊断缓冲，可为空。
 * @return 检查状态，零表示可进入后续注册。
 */
int lepus_abi_check_module(const LepusAbiModuleDescriptor *module, LepusAbiError *error)
{
    size_t at;
    int status;
    if (module == NULL || module->structSize != sizeof(LepusAbiModuleDescriptor))
        return lepus_abi_status(error, LepusAbiInvalidArgument, "invalid module descriptor");
    status = lepus_abi_check(module->abi, error);
    if (status != LepusAbiOk) return status;
    if (module->count > SIZE_MAX / sizeof(LepusAbiRecord) || (module->count != 0u && module->records == NULL))
        return lepus_abi_status(error, LepusAbiInvalidArgument, "invalid module record array");
    for (at = 0; at < module->count; ++at)
    {
        size_t previous;
        const LepusAbiRecord *record = &module->records[at];
        if (record->name == NULL || record->name[0] == '\0' || record->entry == NULL || record->arity < -1)
            return lepus_abi_status(error, LepusAbiInvalidArgument, "invalid module entry");
        for (previous = 0; previous < at; ++previous)
            if (strcmp(module->records[previous].name, record->name) == 0)
                return lepus_abi_status(error, LepusAbiDuplicate, "duplicate module entry");
    }
    return lepus_abi_status(error, LepusAbiOk, NULL);
}

/**
 * @brief 核对实参数量、数组及每个槽的类别。
 * @param frame 调用方保证指向有效存储的帧。
 * @param arity 固定参数数量，-1 表示变参。
 * @param error 调用方有界诊断缓冲，可为空。
 * @return 检查状态，零表示输入合法。
 */
int lepus_abi_check_frame(const LepusAbiFrame *frame, int arity, LepusAbiError *error)
{
    int64_t at;
    if (frame == NULL || arity < -1 || frame->count < 0 || (uint64_t)frame->count > SIZE_MAX / sizeof(int64_t))
        return lepus_abi_status(error, LepusAbiInvalidArgument, "invalid call frame");
    if (arity >= 0 && frame->count != arity)
        return lepus_abi_status(error, LepusAbiInvalidArgument, "argument count mismatch");
    if (frame->count != 0 && (frame->slots == NULL || frame->kinds == NULL))
        return lepus_abi_status(error, LepusAbiInvalidArgument, "missing argument array");
    for (at = 0; at < frame->count; ++at)
        if (frame->kinds[at] != LepusAbiSlotInteger && frame->kinds[at] != LepusAbiSlotFloat)
            return lepus_abi_status(error, LepusAbiInvalidArgument, "invalid argument slot kind");
    return lepus_abi_status(error, LepusAbiOk, NULL);
}

/**
 * @brief 拒绝零与一之外的返回槽类别。
 * @param frame 调用完成后的帧。
 * @param error 调用方有界诊断缓冲，可为空。
 * @return 检查状态，零表示返回类别合法。
 */
int lepus_abi_check_result(const LepusAbiFrame *frame, LepusAbiError *error)
{
    if (frame == NULL || (frame->resultIsFloat != 0 && frame->resultIsFloat != 1))
        return lepus_abi_status(error, LepusAbiInvalidArgument, "invalid result slot kind");
    return lepus_abi_status(error, LepusAbiOk, NULL);
}
