/**
 * @file abi_collection.cpp
 * @brief List/Map/Set/迭代租约的直接 C 入口；借用原始槽，不建立通用值参数数组。
 */
#include "lepus_standard.h"
#include "lepus_abi_internal.h"
namespace
{
/** @brief 与私有存储操作共用唯一清单的入口编号。 */
enum Operation
{
#define LEPUS_COLLECTION_LIST(id, name, arity, apply) id,
#define LEPUS_COLLECTION_HASH(id, name, arity, op)    id,
#include "collection_operations.inc"
#undef LEPUS_COLLECTION_LIST
#undef LEPUS_COLLECTION_HASH
};
/**
 * @brief 在读取任何参数前验证完整 C 帧，随后借用同一运行时执行原语。
 * @tparam Index 唯一清单中的操作下标。
 * @tparam Arity 精确数量，负一表示可变数量。
 * @param frame 当前调用帧。
 */
template <Operation Index, int Arity>
void invoke(LepusAbiFrame *frame)
{
    if (lepus_abi_check_frame(frame, Arity, nullptr) != LepusAbiOk)
    {
        lepus_abi_fail(frame, "invalid collection call frame");
        return;
    }
    lepus_abi_collection_apply(frame, Index);
}
}  // namespace
/**
 * @brief 取得集合存储、程序参数与迭代器的 C 入口表。
 * @param count 接收表项数，可为空。
 * @return 进程期有效的只读表。
 */
extern "C" const LepusAbiRecord *LEPUS_ABI_CALL abi_collection_records(size_t *count)
{
    static const LepusAbiRecord records[] = {
#define LEPUS_COLLECTION_LIST(id, name, arity, apply) { name, invoke<id, arity>, arity },
#define LEPUS_COLLECTION_HASH(id, name, arity, op)    { name, invoke<id, arity>, arity },
#include "collection_operations.inc"
#undef LEPUS_COLLECTION_LIST
#undef LEPUS_COLLECTION_HASH
    };
    if (count) *count = sizeof(records) / sizeof(records[0]);
    return records;
}

#define LEPUS_BUILD_BIND_MODULE collection
#include "aot_bind_module.h"
