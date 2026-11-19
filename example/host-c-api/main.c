/** @file main.c @brief 只包含公开 C 头的宿主注册、调用与释放示例。 */
#include "lepus_abi.h"
#include <stdio.h>
#include <string.h>

static int entered = 0;

/**
 * @brief 把两个已检查的整数槽相加。
 * @param frame 本次同步调用帧。
 */
static void LEPUS_ABI_CALL add(LepusAbiFrame *frame)
{
    int64_t left = 0, right = 0;
    if (!lepus_abi_argument_integer(frame, 0, &left) || !lepus_abi_argument_integer(frame, 1, &right)) return;
    ++entered;
    lepus_abi_return_integer(frame, left + right);
}

/**
 * @brief 报告 C 宿主失败；部分输出不能作为成功结果返回。
 * @param frame 本次同步调用帧。
 */
static void LEPUS_ABI_CALL fail(LepusAbiFrame *frame)
{
    lepus_abi_return_integer(frame, 123);
    lepus_abi_fail(frame, "C host requested failure");
}

/**
 * @brief 故意给出非法返回类别，用于展示边界拒绝。
 * @param frame 本次同步调用帧。
 */
static void LEPUS_ABI_CALL invalid_result(LepusAbiFrame *frame)
{
    frame->result = 123;
    frame->resultIsFloat = 2;
}

/**
 * @brief 运行独立 C 宿主示例并销毁注册表。
 * @return 所有可观察行为符合协议时为零。
 */
int main(void)
{
    char message[256];
    LepusAbiError error = { message, sizeof(message), 0, 0 };
    LepusAbiRegistry *registry = lepus_abi_registry_create(&error);
    LepusAbiInfo abi = lepus_abi_local_info(0);
    char owned_name[] = "host::add";
    const LepusAbiRecord records[] = { { owned_name, add, 2 }, { "host::fail", fail, 0 }, { "host::invalid", invalid_result, 0 } };
    const LepusAbiModuleDescriptor module = { sizeof(module), &abi, records, 3 };
    int64_t values[2] = { 20, 22 };
    unsigned char kinds[2] = { LepusAbiSlotInteger, LepusAbiSlotInteger };
    LepusAbiFrame frame = { NULL, values, kinds, 2, 0, 0 };
    LepusAbiEntry resolved = NULL;
    int32_t arity = 0;
    int failed = 0;
#define REQUIRE(condition)                                             \
    do                                                                 \
    {                                                                  \
        if (!(condition))                                              \
        {                                                              \
            fprintf(stderr, "C API line %d: %s\n", __LINE__, message); \
            failed = 1;                                                \
            goto finish;                                               \
        }                                                              \
    } while (0)
    REQUIRE(registry != NULL);
    REQUIRE(lepus_abi_registry_register(registry, &module, &error) == LepusAbiOk);
    owned_name[0] = 'X'; /* 注册表持有名称副本。 */
    REQUIRE(lepus_abi_registry_resolve(registry, "host::add", &resolved, &arity, &error) == LepusAbiOk);
    REQUIRE(resolved == add && arity == 2);
    REQUIRE(lepus_abi_registry_call(registry, "host::add", &frame, &error) == LepusAbiOk);
    REQUIRE(frame.result == 42 && frame.runtime == NULL && entered == 1);
    frame.count = 1;
    REQUIRE(lepus_abi_registry_call(registry, "host::add", &frame, &error) == LepusAbiInvalidArgument);
    REQUIRE(entered == 1 && frame.result == 0);
    frame.count = 2;
    kinds[1] = LepusAbiSlotFloat;
    REQUIRE(lepus_abi_registry_call(registry, "host::add", &frame, &error) == LepusAbiHostFailure);
    REQUIRE(entered == 1 && frame.result == 0);
    kinds[1] = LepusAbiSlotInteger;
    {
        const LepusAbiRecord conflicting[] = { { "host::temporary", add, 2 }, { "host::add", add, 2 } };
        const LepusAbiModuleDescriptor rejected = { sizeof(rejected), &abi, conflicting, 2 };
        REQUIRE(lepus_abi_registry_register(registry, &rejected, &error) == LepusAbiDuplicate);
        REQUIRE(lepus_abi_registry_resolve(registry, "host::temporary", &resolved, &arity, &error) == LepusAbiNotFound);
        REQUIRE(resolved == NULL && arity == 0);
    }
    frame.count = 0;
    REQUIRE(lepus_abi_registry_call(registry, "host::fail", &frame, &error) == LepusAbiHostFailure);
    REQUIRE(strcmp(message, "C host requested failure") == 0 && frame.result == 0 && frame.runtime == NULL);
    REQUIRE(lepus_abi_registry_call(registry, "host::invalid", &frame, &error) == LepusAbiInvalidArgument);
    REQUIRE(frame.result == 0 && frame.resultIsFloat == 0);
    REQUIRE(lepus_abi_registry_seal(registry, &error) == LepusAbiOk);
    REQUIRE(lepus_abi_registry_seal(registry, &error) == LepusAbiOk);
    REQUIRE(lepus_abi_registry_register(registry, &module, &error) == LepusAbiSealed);
    frame.count = 2;
    REQUIRE(lepus_abi_registry_call(registry, "host::add", &frame, &error) == LepusAbiOk && frame.result == 42);
    puts("opaque C registry: 42; atomic duplicate rejection and failure reset passed");
finish:
    lepus_abi_registry_destroy(registry);
    return failed;
#undef REQUIRE
}
