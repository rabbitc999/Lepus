/** @file lepus_c_api_contract.c @brief 独立 C 宿主的 ABI 协议正反例。 */
#include "abi/lepus_abi.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief 在两个整数槽上执行 C 宿主加法。
 * @param frame 经调用方核对的参数帧。
 */
static void LEPUS_ABI_CALL add(LepusAbiFrame *frame)
{
    frame->result = frame->slots[0] + frame->slots[1];
    frame->resultIsFloat = 0;
}

/**
 * @brief 运行 C/C++ 目标布局互操作及协议拒绝测试。
 * @return 全部断言通过时为零。
 */
int main(void)
{
    LepusAbiInfo local = lepus_abi_local_info(LEPUS_ABI_CAP_SYNC_CALLBACK);
    LepusAbiInfo changed;
    LepusAbiRecord records[2] = { { "host::add", add, 2 }, { "host::other", add, 2 } };
    LepusAbiModuleDescriptor module = { sizeof(LepusAbiModuleDescriptor), &local, records, 2 };
    char text[128];
    LepusAbiError error = { text, sizeof(text), 0, 0 };
    int64_t slots[2] = { 20, 22 };
    unsigned char kinds[2] = { LepusAbiSlotInteger, LepusAbiSlotInteger };
    LepusAbiFrame frame = { NULL, slots, kinds, 2, 0, 0 };
    size_t at;
    assert(lepus_abi_check(&local, &error) == LepusAbiOk);
    assert(error.status == 0 && error.required == 0 && text[0] == '\0');
    assert(lepus_abi_check_module(&module, &error) == LepusAbiOk);
    assert(lepus_abi_check_frame(&frame, records[0].arity, &error) == LepusAbiOk);
    records[0].entry(&frame);
    assert(frame.result == 42 && lepus_abi_check_result(&frame, &error) == LepusAbiOk);
    assert(lepus_abi_check(NULL, NULL) == LepusAbiInvalidArgument);
    changed = local;
    changed.structSize--;
    assert(lepus_abi_check(&changed, &error) == LepusAbiIncompatible);
    changed = local;
    changed.version++;
    assert(lepus_abi_check(&changed, &error) == LepusAbiIncompatible);
    changed = local;
    changed.architecture = local.architecture == 2 ? 4 : 2;
    assert(lepus_abi_check(&changed, &error) == LepusAbiIncompatible);
    changed = local;
    changed.platform = local.platform == 1 ? 2 : 1;
    assert(lepus_abi_check(&changed, &error) == LepusAbiIncompatible);
    changed = local;
    changed.pointerBits /= 2;
    assert(lepus_abi_check(&changed, &error) == LepusAbiIncompatible);
    changed = local;
    changed.littleEndian ^= 1;
    assert(lepus_abi_check(&changed, &error) == LepusAbiIncompatible);
    changed = local;
    changed.callingConvention++;
    assert(lepus_abi_check(&changed, &error) == LepusAbiIncompatible);
    changed = local;
    changed.frameAlignment *= 2;
    assert(lepus_abi_check(&changed, &error) == LepusAbiIncompatible);
    changed = local;
    changed.recordSize++;
    assert(lepus_abi_check(&changed, &error) == LepusAbiIncompatible);
    for (at = 0; at < 6; ++at)
    {
        changed = local;
        changed.frameOffsets[at]++;
        assert(lepus_abi_check(&changed, NULL) == LepusAbiIncompatible);
    }
    for (at = 0; at < 3; ++at)
    {
        changed = local;
        changed.recordOffsets[at]++;
        assert(lepus_abi_check(&changed, NULL) == LepusAbiIncompatible);
    }
    changed = local;
    changed.capabilities = LEPUS_ABI_CAP_BINARY;
    assert(lepus_abi_check(&changed, &error) == LepusAbiOk);
    changed.capabilities = LEPUS_ABI_CAP_RECORD_CLONE;
    assert(lepus_abi_check(&changed, &error) == LepusAbiOk);
    changed.capabilities = LEPUS_ABI_CAP_RECORD_PROBE;
    assert(lepus_abi_check(&changed, &error) == LepusAbiOk);
    changed.capabilities = LEPUS_ABI_CAP_LIST_TEXT;
    assert(lepus_abi_check(&changed, &error) == LepusAbiOk);
    changed.capabilities = LEPUS_ABI_CAP_LIST_NUMBERS;
    assert(lepus_abi_check(&changed, &error) == LepusAbiOk);
    changed.capabilities = LEPUS_ABI_CAP_RECORD_TEXT | LEPUS_ABI_CAP_INPUT_TOKEN | LEPUS_ABI_CAP_TRACKED_TEMPORARY;
    assert(lepus_abi_check(&changed, &error) == LepusAbiOk);
    changed.capabilities = UINT64_C(1) << 63;
    assert(lepus_abi_check(&changed, &error) == LepusAbiIncompatible);
    assert(strstr(text, "capability") != NULL);
    {
        char bounded[5] = { 'x', 'x', 'x', 'x', '!' };
        LepusAbiError small = { bounded, 4, 0, 0 };
        assert(lepus_abi_check(NULL, &small) == LepusAbiInvalidArgument);
        assert(small.required == strlen("missing ABI descriptor") + 1);
        assert(bounded[3] == '\0' && bounded[4] == '!');
        small.data = NULL;
        small.capacity = 0;
        assert(lepus_abi_check(NULL, &small) == LepusAbiInvalidArgument);
        assert(lepus_abi_check(&local, &small) == 0 && small.required == 0);
    }
    records[1].name = records[0].name;
    assert(lepus_abi_check_module(&module, &error) == LepusAbiDuplicate);
    records[1].name = "";
    assert(lepus_abi_check_module(&module, &error) == LepusAbiInvalidArgument);
    records[1].name = "host::other";
    records[1].entry = NULL;
    assert(lepus_abi_check_module(&module, &error) == LepusAbiInvalidArgument);
    records[1].entry = add;
    records[1].arity = -2;
    assert(lepus_abi_check_module(&module, &error) == LepusAbiInvalidArgument);
    module.records = NULL;
    assert(lepus_abi_check_module(&module, &error) == LepusAbiInvalidArgument);
    module.count = 0;
    assert(lepus_abi_check_module(&module, &error) == LepusAbiOk);
    assert(lepus_abi_check_module(NULL, &error) == LepusAbiInvalidArgument);
    module.abi = NULL;
    assert(lepus_abi_check_module(&module, NULL) == LepusAbiInvalidArgument);
    assert(lepus_abi_check_frame(NULL, 0, &error) == LepusAbiInvalidArgument);
    assert(lepus_abi_check_frame(&frame, 1, &error) == LepusAbiInvalidArgument);
    frame.count = -1;
    assert(lepus_abi_check_frame(&frame, -1, &error) == LepusAbiInvalidArgument);
    frame.count = INT64_MAX;
    assert(lepus_abi_check_frame(&frame, -1, &error) == LepusAbiInvalidArgument);
    frame.count = 2;
    frame.slots = NULL;
    assert(lepus_abi_check_frame(&frame, 2, &error) == LepusAbiInvalidArgument);
    frame.slots = slots;
    frame.kinds = NULL;
    assert(lepus_abi_check_frame(&frame, 2, &error) == LepusAbiInvalidArgument);
    frame.kinds = kinds;
    kinds[1] = 2;
    assert(lepus_abi_check_frame(&frame, 2, &error) == LepusAbiInvalidArgument);
    kinds[1] = LepusAbiSlotFloat;
    assert(lepus_abi_check_frame(&frame, -1, &error) == LepusAbiOk);
    frame.resultIsFloat = 2;
    assert(lepus_abi_check_result(&frame, &error) == LepusAbiInvalidArgument);
    frame.resultIsFloat = 1;
    assert(lepus_abi_check_result(&frame, &error) == LepusAbiOk);
    assert(lepus_abi_check_result(NULL, &error) == LepusAbiInvalidArgument);
    frame.count = 0;
    frame.slots = NULL;
    frame.kinds = NULL;
    assert(lepus_abi_check_frame(&frame, 0, &error) == LepusAbiOk);
    puts("C ABI target, module, frame and bounded error contracts passed");
    return 0;
}
