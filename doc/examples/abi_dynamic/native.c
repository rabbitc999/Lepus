#include <stdint.h>

#if defined(_WIN32)
#define LEPUS_ABI_EXPORT __declspec(dllexport)
#else
#define LEPUS_ABI_EXPORT
#endif

LEPUS_ABI_EXPORT int32_t abi_add(int32_t left, int32_t right)
{
    return left + right;
}

LEPUS_ABI_EXPORT const char *abi_name(void)
{
    return "native";
}
