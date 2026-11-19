/** @file abi_compress.cpp @brief lztext 内核的 C 帧入口，不依赖旧模板胶水。 */
#include "lepus_standard.h"
#include "../lztext/lztext.hpp"
#include <exception>
#include <vector>
#include <string>

/**
 * @brief 借用完整字符串后压缩，并复制为独立拥有的字节列表。
 * @param frame 包含一个 String 句柄，返回 List<uint8> 句柄。
 */
static void LEPUS_ABI_CALL compressEntry(LepusAbiFrame *frame)
try
{
    int64_t handle = 0;
    if (!lepus_abi_argument_integer(frame, 0, &handle)) return;
    const unsigned char *data = nullptr;
    size_t length = 0;
    if (!lepus_abi_bytes(frame, handle, &data, &length)) return;
    const unsigned char empty = 0;
    const auto packed = lztext::compress(data ? data : &empty, length);
    lepus_abi_return_integer(frame, lepus_abi_list_from_bytes(frame, packed.data(), packed.size()));
} catch (const std::exception &error) { lepus_abi_fail(frame, error.what()); } catch (...)
{
    lepus_abi_fail(frame, "文本压缩内核失败");
}

/**
 * @brief 将字节列表复制为连续输入，保留解压内容中的零字节与原始字节。
 * @param frame 包含 List<uint8> 句柄，返回拥有型 String。
 */
static void LEPUS_ABI_CALL decompressEntry(LepusAbiFrame *frame)
try
{
    int64_t handle = 0;
    if (!lepus_abi_argument_integer(frame, 0, &handle)) return;
    size_t length = 0;
    if (!lepus_abi_list_bytes(frame, handle, nullptr, 0, &length)) return;
    std::vector<unsigned char> packed(length);
    if (!lepus_abi_list_bytes(frame, handle, packed.data(), packed.size(), &length)) return;
    std::string plain;
    if (!lztext::decompress(packed.data(), packed.size(), plain))
    {
        lepus_abi_fail(frame, "std::compress::decompress：数据不是有效的 lztext 流或已损坏");
        return;
    }
    lepus_abi_return_integer(frame, lepus_abi_string_bytes(frame, plain.data(), plain.size()));
} catch (const std::exception &error) { lepus_abi_fail(frame, error.what()); } catch (...)
{
    lepus_abi_fail(frame, "文本解压内核失败");
}

/**
 * @brief 取得文本压缩内核的只读 C 函数表。
 * @param count 接收入口数量，可为空。
 * @return 程序存活期间有效的入口表。
 */
extern "C" const LepusAbiRecord *LEPUS_ABI_CALL abi_compress_records(size_t *count)
{
    static const LepusAbiRecord records[] = {
        { "std::compress::compress", compressEntry, 1 },
        { "std::compress::decompress", decompressEntry, 1 },
    };
    if (count) *count = sizeof(records) / sizeof(records[0]);
    return records;
}

#ifndef GLUE_NO_STD_COMPRESS
#define LEPUS_BUILD_BIND_MODULE compress
#include "aot_bind_module.h"
#endif
