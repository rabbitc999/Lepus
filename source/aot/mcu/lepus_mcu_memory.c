#include "lepus_mcu_memory.h"
#include "lepus_mcu_hal.h"

typedef struct
{
    uint64_t destination, source, size;
} LepusDataSegment;

typedef struct LepusBlock
{
    struct LepusBlock *next;
    uint64_t address;
    size_t size;
    _Alignas(16) unsigned char data[];
} LepusBlock;

extern const unsigned char lepus_data_image[];
extern const LepusDataSegment lepus_data_segments[];
extern const uint64_t lepus_data_segment_count, lepus_data_size, lepus_heap_size;
extern void lepus_reset_slots(void);
extern void lepus_reset_address_view(void);
extern _Noreturn void lepus_panic(const char *);

static unsigned char *data;
static LepusBlock *blocks;
static uint64_t nextAddress;
static uint64_t liveBytes;

/**
 * @brief 建立零填充数据区，装入稀疏片段并重置编译器槽。
 */
void lepus_host_init(void)
{
    if (data || lepus_data_size > SIZE_MAX) lepus_panic("MCU: invalid data image");
    size_t size = (size_t)lepus_data_size;
    data = lepus_mcu_allocate(size ? size : 1);
    if (!data) lepus_panic("MCU: data allocation failed");
    for (size_t i = 0; i < size; ++i) data[i] = 0;
    for (uint64_t i = 0; i < lepus_data_segment_count; ++i)
    {
        const LepusDataSegment *segment = &lepus_data_segments[i];
        if (segment->destination > size || segment->size > size - segment->destination)
            lepus_panic("MCU: invalid data segment");
        for (uint64_t j = 0; j < segment->size; ++j)
            data[segment->destination + j] = lepus_data_image[segment->source + j];
    }
    nextAddress = (lepus_data_size + 15u) & ~(uint64_t)15u;
    if (nextAddress == 0) nextAddress = 16;
    lepus_reset_slots();
    lepus_reset_address_view();
}

/**
 * @brief 清除地址缓存，释放全部仍存活的堆块和静态数据。
 */
void lepus_host_finish(void)
{
    lepus_reset_address_view();
    while (blocks) lepus_host_drop((int64_t)blocks->address);
    lepus_mcu_free(data);
    data = NULL;
}

/**
 * @brief 查找整个访问范围所在的存活数据区或堆块。
 * @param address 逻辑字节地址。
 * @param count 访问长度。
 * @return 地址对应的物理指针。
 */
unsigned char *lepus_host_address(int64_t address, uint64_t count)
{
    if (!data || address < 0) lepus_panic("MCU: invalid address");
    const uint64_t offset = (uint64_t)address;
    if (offset < lepus_data_size && count <= lepus_data_size - offset)
        return data + (size_t)offset;
    for (LepusBlock *block = blocks; block; block = block->next)
    {
        if (offset < block->address) continue;
        const uint64_t within = offset - block->address;
        if (within <= block->size && count <= block->size - within)
            return block->data + (size_t)within;
    }
    lepus_panic("MCU: invalid or expired range");
}

/**
 * @brief 通过板级堆分配存活块，逻辑基址在一次运行中不复用。
 * @param bytes 字节数，零按一个字节处理。
 * @return 16 字节对齐的逻辑基址。
 */
int64_t lepus_host_allocate(uint64_t bytes)
{
    if (bytes == 0) bytes = 1;
    if (!data || bytes > SIZE_MAX - sizeof(LepusBlock) ||
        bytes > lepus_heap_size - liveBytes || nextAddress > INT64_MAX - 16 ||
        bytes > INT64_MAX - 16 - nextAddress)
        lepus_panic("MCU: heap budget exceeded");
    LepusBlock *block = lepus_mcu_allocate(sizeof(*block) + (size_t)bytes);
    if (!block) lepus_panic("MCU: heap allocation failed");
    block->next = blocks;
    block->address = nextAddress;
    block->size = (size_t)bytes;
    for (size_t i = 0; i < block->size; ++i) block->data[i] = 0;
    blocks = block;
    liveBytes += bytes;
    nextAddress += (bytes + 16u) & ~(uint64_t)15u;
    return (int64_t)block->address;
}

/**
 * @brief 撤销基址对应的块并归还板级堆。
 * @param address 块基址，零表示空句柄。
 */
void lepus_host_drop(int64_t address)
{
    if (!address) return;
    for (LepusBlock **link = &blocks; *link; link = &(*link)->next)
    {
        LepusBlock *block = *link;
        if (block->address != (uint64_t)address) continue;
        *link = block->next;
        liveBytes -= block->size;
        lepus_reset_address_view();
        lepus_mcu_free(block);
        return;
    }
    lepus_panic("MCU: invalid heap release");
}
