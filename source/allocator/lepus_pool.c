#ifndef PARSEENGINE_LEPUS_POOL_IMPLEMENTATION
#define PARSEENGINE_LEPUS_POOL_IMPLEMENTATION
#include "lepus_pool.h"
#include <stdlib.h>
#include <string.h>

#define LEPUS_POOL_NONE      SIZE_MAX
static inline int lepus_pool_valid_alignment(size_t a) { return a && !(a & (a - 1)); }
static inline int lepus_pool_owns(const McPool *p, const void *pointer)
{
    uintptr_t address = (uintptr_t)pointer, start;
    if (!p || !p->storage || !pointer) return 0;
    start = (uintptr_t)p->storage;
    return address >= start && address - start < p->capacity;
}
#if LEPUS_ALLOCATOR_TYPE == LEPUS_ALLOCATOR_LINKED
/* 对齐：块头 16 字节、块大小按 16 字节向上取整，
 * 于是「块首 16 对齐 ⇒ 载荷 16 对齐」，常见的 max_align_t 请求不必走对齐特例。 */
#define LEPUS_POOL_ALIGNMENT ((size_t)16)
/* ---------------------------------------------------------------------------
 *  TLSF：两级隔离适配（Two-Level Segregated Fit）
 *
 *  这是按 TLSF 论文（Masmano/Ripoll/Crespo/Real, ECRTS 2004）与 TLSF 2.0 的
 *  块头压缩思路自己实现的版本，没有引入上游源码。选择它是因为我们的约束正好
 *  是它被设计出来要解决的问题：
 *
 *    - 只在调用方给的固定缓冲区上工作，不向系统要内存；
 *    - 分配与释放都是 **O(1)**（两级位图定位 + 定长双向链表），不随块数增长；
 *    - 每次分配只有一个 16 字节块头，且块头里只有「物理前邻 + 大小 + 2 位标志」，
 *      空闲链指针放在空闲块自己的载荷里，使用中的块完全不吃这部分；
 *    - 32 个二级细分把大块的内部碎片压到 ~3%，不像首次适配那样把大块切碎。
 *
 *  上一版是「边界标记 + 显式空闲双向链表 + 首次适配」：每块 48 字节块头 + 8 字节
 *  回指，50 万个小对象就是 44 MB（有效数据才 12 MB），而且首次适配要线性扫空闲
 *  链表，512 MiB 池被切碎后连 80 MB 连续块都找不到。见 doc/20-memory-allocator.md。
 * ------------------------------------------------------------------------- */

typedef struct McBlock
{
    struct McBlock *prev_phys; /* 物理相邻的前一块；只有 PREV_FREE 置位时可信 */
    size_t size;               /* 块大小（含本块头），低 2 位是标志 */
} McBlock;

#define LEPUS_BLOCK_FREE      ((size_t)1) /* 本块空闲 */
#define LEPUS_BLOCK_PREV_FREE ((size_t)2) /* 物理前邻空闲 */
#define LEPUS_BLOCK_FLAGS     ((size_t)3)
#define LEPUS_BLOCK_OVERHEAD  (sizeof(McBlock))                    /* 16 字节 */
#define LEPUS_BLOCK_MIN       (sizeof(McBlock) + 2u * sizeof(void *)) /* 空闲块要放得下两个链指针 */

static inline size_t lepus_block_size(const McBlock *b) { return b->size & ~LEPUS_BLOCK_FLAGS; }
static inline void lepus_block_set_size(McBlock *b, size_t size) { b->size = size | (b->size & LEPUS_BLOCK_FLAGS); }
static inline int lepus_block_is_free(const McBlock *b) { return (b->size & LEPUS_BLOCK_FREE) != 0; }
static inline void lepus_block_set_free(McBlock *b, int on) { b->size = on ? (b->size | LEPUS_BLOCK_FREE) : (b->size & ~LEPUS_BLOCK_FREE); }
static inline int lepus_block_prev_is_free(const McBlock *b) { return (b->size & LEPUS_BLOCK_PREV_FREE) != 0; }
static inline void lepus_block_set_prev_free(McBlock *b, int on) { b->size = on ? (b->size | LEPUS_BLOCK_PREV_FREE) : (b->size & ~LEPUS_BLOCK_PREV_FREE); }
static inline unsigned char *lepus_block_data(McBlock *b) { return (unsigned char *)b + sizeof(McBlock); }
static inline McBlock *lepus_block_from_data(void *pointer) { return (McBlock *)((unsigned char *)pointer - sizeof(McBlock)); }
static inline McBlock *lepus_block_next(const McBlock *b) { return (McBlock *)((unsigned char *)b + lepus_block_size(b)); }
static inline McBlock **lepus_link_next(McBlock *b) { return (McBlock **)lepus_block_data(b); }
static inline McBlock **lepus_link_prev(McBlock *b) { return (McBlock **)(lepus_block_data(b) + sizeof(void *)); }
/**
 * @brief 最高置位下标（0 基），参数必须非 0。
 */
static inline int lepus_bit_high(size_t x)
{
#if defined(_MSC_VER)
    unsigned long index;
#if defined(_WIN64)
    _BitScanReverse64(&index, (unsigned __int64)x);
#else
    _BitScanReverse(&index, (unsigned long)x);
#endif
    return (int)index;
#elif defined(__GNUC__) || defined(__clang__)
    /* 注意 MinGW 的 `unsigned long` 只有 32 位，必须用 long long 的版本，
     * 否则 64 位平台上算出来的下标会整体偏移。 */
    return (int)(sizeof(unsigned long long) * 8u - 1u - (unsigned)__builtin_clzll((unsigned long long)x));
#else
    int index = -1;
    while (x) { x >>= 1; ++index; }
    return index;
#endif
}
/**
 * @brief 最低置位下标，参数必须非 0。
 */
static inline int lepus_bit_low(size_t x)
{
#if defined(_MSC_VER)
    unsigned long index;
#if defined(_WIN64)
    _BitScanForward64(&index, (unsigned __int64)x);
#else
    _BitScanForward(&index, (unsigned long)x);
#endif
    return (int)index;
#elif defined(__GNUC__) || defined(__clang__)
    return (int)__builtin_ctzll((unsigned long long)x);
#else
    int index = 0;
    while (!(x & 1u)) { x >>= 1; ++index; }
    return index;
#endif
}
/* 二级细分的位宽与一级起点：256 字节以下算小块，直接按 16 字节档位落格。 */
#define LEPUS_SL_LOG2   5u
#define LEPUS_ALIGN_LOG2 4u
#define LEPUS_SMALL_BLOCK ((size_t)1 << (LEPUS_SL_LOG2 + LEPUS_ALIGN_LOG2))
#define LEPUS_FL_SHIFT   (LEPUS_SL_LOG2 + LEPUS_ALIGN_LOG2)
/**
 * @brief 把块大小映射到（一级, 二级）槽位。
 */
static inline void lepus_mapping_insert(size_t size, int *fl, int *sl)
{
    if (size < LEPUS_SMALL_BLOCK)
    {
        *fl = 0;
        *sl = (int)(size >> LEPUS_ALIGN_LOG2);
    }
    else
    {
        const int high = lepus_bit_high(size);
        *sl = (int)((size >> (high - LEPUS_SL_LOG2)) ^ LEPUS_SL_COUNT);
        *fl = high - (int)(LEPUS_FL_SHIFT - 1u);
    }
}
/**
 * @brief 找「够用」的槽位：先按请求大小向上取整到本档，避免落到刚好不够的格子。
 */
static inline void lepus_mapping_search(size_t size, int *fl, int *sl)
{
    if (size >= LEPUS_SMALL_BLOCK)
    {
        const size_t round = ((size_t)1 << (lepus_bit_high(size) - LEPUS_SL_LOG2)) - 1u;
        size += round;
    }
    lepus_mapping_insert(size, fl, sl);
}
/**
 * @brief 把空闲块挂进对应槽位。
 */
static void lepus_insert_block(McPool *p, McBlock *b)
{
    int fl = 0, sl = 0;
    McBlock *head;
    lepus_mapping_insert(lepus_block_size(b), &fl, &sl);
    head = (McBlock *)p->free_lists[fl][sl];
    *lepus_link_next(b) = head;
    *lepus_link_prev(b) = NULL;
    if (head) *lepus_link_prev(head) = b;
    p->free_lists[fl][sl] = b;
    p->fl_bitmap |= (uint32_t)1 << fl;
    p->sl_bitmap[fl] |= (uint32_t)1 << sl;
    lepus_block_set_free(b, 1);
}
/**
 * @brief 把空闲块从槽位上摘下来。
 */
static void lepus_remove_block(McPool *p, McBlock *b)
{
    int fl = 0, sl = 0;
    McBlock *next, *prev;
    lepus_mapping_insert(lepus_block_size(b), &fl, &sl);
    next = *lepus_link_next(b);
    prev = *lepus_link_prev(b);
    if (!prev)
        p->free_lists[fl][sl] = next;
    else
        *lepus_link_next(prev) = next;
    if (next) *lepus_link_prev(next) = prev;
    if (!p->free_lists[fl][sl])
    {
        p->sl_bitmap[fl] &= ~((uint32_t)1 << sl);
        if (!p->sl_bitmap[fl]) p->fl_bitmap &= ~((uint32_t)1 << fl);
    }
    lepus_block_set_free(b, 0);
}
/**
 * @brief 两级位图定位一个够大的空闲块（不摘链）。
 * @return 找不到返回 NULL。
 */
static McBlock *lepus_find_block(McPool *p, size_t size)
{
    int fl = 0, sl = 0;
    uint32_t sl_map, fl_map;
    lepus_mapping_search(size, &fl, &sl);
    if (fl >= (int)LEPUS_FL_COUNT) return NULL;
    sl_map = p->sl_bitmap[fl] & (~(uint32_t)0 << sl);
    if (!sl_map)
    {
        if (fl + 1 >= (int)LEPUS_FL_COUNT)
            fl_map = 0;
        else
            fl_map = p->fl_bitmap & (~(uint32_t)0 << (fl + 1));
        if (!fl_map) return NULL;
        fl = lepus_bit_low(fl_map);
        sl_map = p->sl_bitmap[fl];
        if (!sl_map) return NULL;
    }
    sl = lepus_bit_low(sl_map);
    return (McBlock *)p->free_lists[fl][sl];
}
/**
 * @brief 记一笔活动占用（统计口径：含块头与内部碎片）。
 */
static inline void lepus_account_alloc(McPool *p, size_t size)
{
    p->stats.live_bytes += size;
    ++p->stats.live_blocks;
    ++p->stats.total_blocks;
    if (p->stats.live_bytes > p->stats.peak_bytes) p->stats.peak_bytes = p->stats.live_bytes;
}
static inline int lepus_pool_init(McPool *p, void *storage, size_t capacity)
{
    uintptr_t start, aligned;
    size_t skipped, raw_capacity = capacity;
    McBlock *first, *last;
    if (!p) return 0;
    memset(p, 0, sizeof(*p));
    p->free_head = LEPUS_POOL_NONE;
    if (!storage) return 0;
    start = (uintptr_t)storage;
    aligned = (start + (LEPUS_POOL_ALIGNMENT - 1)) & ~(uintptr_t)(LEPUS_POOL_ALIGNMENT - 1);
    skipped = (size_t)(aligned - start);
    if (capacity < skipped) return 0;
    capacity = (capacity - skipped) & ~(LEPUS_POOL_ALIGNMENT - 1);
    /* 至少要放得下一块空闲块，外加尾部哨兵块头。 */
    if (capacity < LEPUS_BLOCK_MIN + LEPUS_BLOCK_OVERHEAD) return 0;
    p->storage = (unsigned char *)aligned;
    p->capacity = capacity;
    p->raw_storage = storage;
    p->raw_capacity = raw_capacity;
    first = (McBlock *)p->storage;
    first->prev_phys = NULL;
    first->size = capacity - LEPUS_BLOCK_OVERHEAD;
    lepus_block_set_free(first, 1);
    lepus_block_set_prev_free(first, 0);
    /* 尾部哨兵：大小 0、使用中，于是「算下一块」永远有落点，不用到处判越界。 */
    last = lepus_block_next(first);
    last->prev_phys = first;
    last->size = LEPUS_BLOCK_PREV_FREE;
    lepus_insert_block(p, first);
    return 1;
}
/**
 * @brief 把一块切成「正好 size 用 + 余下空闲」，余量不够一块就整块给出。
 */
static void lepus_split_block(McPool *p, McBlock *b, size_t size)
{
    const size_t total = lepus_block_size(b);
    McBlock *next = lepus_block_next(b); /* 改 size 之前算好 */
    if (total - size >= LEPUS_BLOCK_MIN)
    {
        McBlock *rest = (McBlock *)((unsigned char *)b + size);
        rest->prev_phys = b;
        rest->size = total - size;
        /* 余下那块的物理前邻是刚切出去、正在用的那块，所以 PREV_FREE 必须是 0。 */
        lepus_block_set_prev_free(rest, 0);
        lepus_block_set_size(b, size);
        lepus_block_set_free(b, 0);
        next->prev_phys = rest;
        lepus_block_set_prev_free(next, 1);
        lepus_insert_block(p, rest);
        return;
    }
    lepus_block_set_prev_free(next, 0);
}
/**
 * @brief 摘下一个够大的空闲块。
 */
static McBlock *lepus_take_block(McPool *p, size_t size)
{
    McBlock *b = lepus_find_block(p, size);
    if (!b) return NULL;
    lepus_remove_block(p, b);
    return b;
}
static inline void *lepus_pool_allocate(McPool *p, size_t bytes, size_t alignment)
{
    size_t want;
    McBlock *block;
    if (!p || !p->storage || !lepus_pool_valid_alignment(alignment)) return NULL;
    if (!bytes) bytes = 1;
    if (bytes > p->capacity) return NULL;
    /* 池只保证 16 字节对齐（块头 16 字节、载荷紧跟在块头后面）：
     * 更宽的对齐请上层自行缓冲，这一层不再做对齐特例。 */
    if (alignment > LEPUS_POOL_ALIGNMENT) return NULL;
    want = (bytes + LEPUS_BLOCK_OVERHEAD + LEPUS_POOL_ALIGNMENT - 1) & ~(LEPUS_POOL_ALIGNMENT - 1);
    if (want < LEPUS_BLOCK_MIN) want = LEPUS_BLOCK_MIN;
    block = lepus_take_block(p, want);
    if (!block) return NULL;
    lepus_split_block(p, block, want);
    lepus_account_alloc(p, lepus_block_size(block));
    return lepus_block_data(block);
}
static inline int lepus_pool_release(McPool *p, void *pointer)
{
    McBlock *block, *next;
    size_t offset, size;
    if (!p || !p->storage || !pointer || !lepus_pool_owns(p, pointer)) return 0;
    offset = (size_t)((unsigned char *)pointer - p->storage);
    if (offset < LEPUS_BLOCK_OVERHEAD || offset % LEPUS_POOL_ALIGNMENT) return 0; /* 内部指针 */
    block = lepus_block_from_data(pointer);
    size = lepus_block_size(block);
    if (size < LEPUS_BLOCK_MIN || size % LEPUS_POOL_ALIGNMENT || offset + size > p->capacity) return 0;
    /* 块头的物理前邻必须真的「跨一步」踩到本块：内部指针或已合并的旧指针
     * 在这里就对不上，不用为每块多存一个回指。首块的 offset 正好等于块头大小。 */
    if (offset != LEPUS_BLOCK_OVERHEAD)
    {
        McBlock *prev = block->prev_phys;
        if (!prev || (unsigned char *)prev < p->storage || lepus_block_next(prev) != block) return 0;
    }
    if (lepus_block_is_free(block)) return 0; /* 重复释放 */
    p->stats.live_bytes -= size;
    --p->stats.live_blocks;
    next = lepus_block_next(block);
    if (lepus_block_is_free(next))
    {
        lepus_remove_block(p, next);
        lepus_block_set_size(block, size + lepus_block_size(next));
    }
    if (lepus_block_prev_is_free(block))
    {
        McBlock *prev = block->prev_phys;
        if (prev && (unsigned char *)prev >= p->storage)
        {
            lepus_remove_block(p, prev);
            lepus_block_set_size(prev, lepus_block_size(prev) + lepus_block_size(block));
            block = prev;
        }
    }
    next = lepus_block_next(block);
    next->prev_phys = block;
    lepus_block_set_prev_free(next, 1);
    lepus_insert_block(p, block);
    return 1;
}
#else /* LEPUS_ALLOCATOR_TYPE == LEPUS_ALLOCATOR_LINEAR */
/* 线性（bump）实现：只向后切块，释放只把块记成墓碑，地址到 reset 前不复用。 */
typedef struct McPoolBlock
{
    size_t size, previous_size, payload, requested, next_free, previous_free;
} McPoolBlock;
#define LEPUS_POOL_ALIGNMENT ((size_t)sizeof(void *))
static inline McPoolBlock lepus_pool_block(const McPool *p, size_t at)
{
    McPoolBlock b;
    memcpy(&b, p->storage + at, sizeof(b));
    return b;
}
static inline void lepus_pool_store(McPool *p, size_t at, McPoolBlock b) { memcpy(p->storage + at, &b, sizeof(b)); }
static inline int lepus_pool_init(McPool *p, void *storage, size_t capacity)
{
    uintptr_t start, aligned;
    size_t skipped, raw_capacity = capacity;
    McPoolBlock b;
    if (!p) return 0;
    memset(p, 0, sizeof(*p));
    p->free_head = LEPUS_POOL_NONE;
    if (!storage) return 0;
    start = (uintptr_t)storage;
    if (start > UINTPTR_MAX - (LEPUS_POOL_ALIGNMENT - 1)) return 0;
    aligned = (start + LEPUS_POOL_ALIGNMENT - 1) & ~(uintptr_t)(LEPUS_POOL_ALIGNMENT - 1);
    skipped = (size_t)(aligned - start);
    if (capacity < skipped) return 0;
    capacity = (capacity - skipped) & ~(LEPUS_POOL_ALIGNMENT - 1);
    if (capacity < sizeof(b) + sizeof(size_t) + LEPUS_POOL_ALIGNMENT) return 0;
    p->storage = (unsigned char *)aligned;
    p->capacity = capacity;
    p->raw_storage = storage;
    p->raw_capacity = raw_capacity;
    p->free_head = 0; /* 线性实现：free_head 当 bump 游标 */
    return 1;
}
static inline void *lepus_pool_allocate(McPool *p, size_t bytes, size_t alignment)
{
    size_t at, avail, wanted;
    uintptr_t base, address;
    size_t payload;
    McPoolBlock b;
    if (!p || !p->storage || !lepus_pool_valid_alignment(alignment)) return NULL;
    if (!bytes) bytes = 1;
    at = p->free_head; /* 线性实现：free_head 当 bump 游标（已对齐到 LEPUS_POOL_ALIGNMENT） */
    if (at > p->capacity) return NULL;
    avail = p->capacity - at;
    if (sizeof(b) + sizeof(size_t) > avail) return NULL;
    base = (uintptr_t)(p->storage + at);
    if (base > UINTPTR_MAX - sizeof(b) - sizeof(size_t) - (alignment - 1)) return NULL;
    address = (base + sizeof(b) + sizeof(size_t) + alignment - 1) & ~(uintptr_t)(alignment - 1);
    payload = (size_t)(address - base);
    if (payload > avail || bytes > avail - payload) return NULL;
    if (payload + bytes > SIZE_MAX - (LEPUS_POOL_ALIGNMENT - 1)) return NULL;
    wanted = (payload + bytes + LEPUS_POOL_ALIGNMENT - 1) & ~(LEPUS_POOL_ALIGNMENT - 1);
    if (wanted > avail) return NULL;
    memset(&b, 0, sizeof(b));
    b.size = wanted;
    b.previous_size = at;
    b.payload = payload;
    b.requested = bytes;
    lepus_pool_store(p, at, b);
    memcpy((void *)(address - sizeof(size_t)), &at, sizeof(at));
    p->free_head = at + wanted;
    p->stats.live_bytes += b.size;
    ++p->stats.live_blocks;
    ++p->stats.total_blocks;
    if (p->stats.live_bytes > p->stats.peak_bytes) p->stats.peak_bytes = p->stats.live_bytes;
    return (void *)address;
}
static inline int lepus_pool_release(McPool *p, void *pointer)
{
    size_t at;
    McPoolBlock b;
    if (!lepus_pool_owns(p, pointer) || (uintptr_t)pointer - (uintptr_t)p->storage < sizeof(b) + sizeof(size_t)) return 0;
    memcpy(&at, (unsigned char *)pointer - sizeof(size_t), sizeof(at));
    if (at % LEPUS_POOL_ALIGNMENT || at > p->capacity - sizeof(b)) return 0;
    b = lepus_pool_block(p, at);
    if (!b.payload || b.size < sizeof(b) + sizeof(size_t) || b.size > p->capacity - at || b.payload >= b.size || p->storage + at + b.payload != pointer) return 0;
    p->stats.live_bytes -= b.size;
    --p->stats.live_blocks;
    /* 墓碑：地址保留记录但不再复用，直到 reset / 整池清零。 */
    b.payload = b.requested = 0;
    lepus_pool_store(p, at, b);
    return 1;
}
#endif
static inline int lepus_pool_finish(McPool *p, McPoolStats *stats)
{
    size_t i;
    volatile unsigned char *buffer;
    if (!p || !p->storage || p->stats.live_blocks) return 0;
    if (stats) *stats = p->stats;
    buffer = (volatile unsigned char *)p->raw_storage;
    for (i = 0; i < p->raw_capacity; ++i) buffer[i] = 0;
    memset(p, 0, sizeof(*p));
    p->free_head = LEPUS_POOL_NONE;
    return 1;
}
#endif
