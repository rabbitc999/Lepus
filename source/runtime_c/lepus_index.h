/** @file lepus_index.h @brief 64 位键到槽号的 C 开放寻址索引。 */
#ifndef LEPUS_C_INDEX_H
#define LEPUS_C_INDEX_H
#include "lepus_buffer.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/** @brief 状态、键和槽号均保存在捕获分配器拥有的连续 C 存储中。 */
typedef struct LepusCIndex
{
    LepusCBuffer slots;
    size_t length, deleted;
} LepusCIndex;
/**
 * @brief 初始化空索引。
 * @param index 未持有存储的目标。
 * @param allocator 可选分配器。
 * @return 操作状态。
 */
LepusCStatus lepus_c_index_init(LepusCIndex *index, const LepusCAllocator *allocator);
/**
 * @brief 查找完整 64 位键。
 * @param index 已初始化索引。
 * @param key 键位模式。
 * @param value 接收槽号；允许为空。
 * @return 找到为一，否则为零。
 */
int lepus_c_index_get(const LepusCIndex *index, uint64_t key, size_t *value);
/**
 * @brief 插入或替换槽号，分配失败时原索引不变。
 * @param index 索引。
 * @param key 键。
 * @param value 槽号。
 * @return 操作状态。
 */
LepusCStatus lepus_c_index_set(LepusCIndex *index, uint64_t key, size_t value);
/**
 * @brief 删除键，保留其余查询的探测链。
 * @param index 索引。
 * @param key 键。
 * @return 原先存在为一，否则为零。
 */
int lepus_c_index_remove(LepusCIndex *index, uint64_t key);
/**
 * @brief 复制到目标原分配器，失败保持目标不变。
 * @param target 已初始化目标。
 * @param source 已初始化源。
 * @return 操作状态。
 */
LepusCStatus lepus_c_index_copy(LepusCIndex *target, const LepusCIndex *source);
/**
 * @brief 清空键但保留容量。
 * @param index 索引。
 */
void lepus_c_index_clear(LepusCIndex *index);
/**
 * @brief 向原所有者归还全部存储。
 * @param index 索引。
 */
void lepus_c_index_destroy(LepusCIndex *index);
#ifdef __cplusplus
}
#endif
#endif
