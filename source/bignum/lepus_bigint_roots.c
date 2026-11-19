/**
 * @file lepus_bigint_roots.c
 * @brief 保持原二分边界及乘除顺序的整数根 C 实现。
 */
#include "lepus_bigint_internal.h"
#define CBN_TRY(expression)                     \
    do                                          \
    {                                           \
        status = (expression);                  \
        if (status != LEPUS_C_OK) goto cleanup; \
    } while (0)
/**
 * @brief 共享二分工作区，保留平方根与一般根各自的上界和幂计算策略。
 * @param output 结果。
 * @param value 非负数。
 * @param degree 正根次。
 * @param square 是否使用原平方根入口的倍增策略。
 * @return 操作状态。
 */
static LepusCStatus root_integer(LepusBigInt *output, const LepusBigInt *value, uint64_t degree, int square)
{
    LepusBigInt work[7];
    LepusBigInt *low = &work[0], *high = &work[1], *middle = &work[2], *power = &work[3];
    LepusBigInt *one = &work[4], *two = &work[5], *difference = &work[6];
    LepusCStatus status = LEPUS_C_OK;
    size_t index;
    if (!output || !value || !degree) return LEPUS_C_INVALID_ARGUMENT;
    if (value->sign < 0) return LEPUS_C_DOMAIN_ERROR;
    if (!value->sign || degree == 1) return lepus_bigint_copy(output, value);
    for (index = 0; index < 7; ++index) cbn_temporary(&work[index], output);
    CBN_TRY(lepus_bigint_set_u64(one, 1));
    CBN_TRY(lepus_bigint_set_u64(two, 2));
    CBN_TRY(lepus_bigint_set_u64(high, 1));
    if (square)
    {
        for (;;)
        {
            CBN_TRY(lepus_bigint_multiply(power, high, high));
            if (lepus_bigint_compare(power, value) > 0) break;
            CBN_TRY(lepus_bigint_multiply(high, high, two));
        }
    }
    else
        CBN_TRY(lepus_bigint_shift_left(high, lepus_bigint_bit_length(value) / degree + 1));
    for (;;)
    {
        CBN_TRY(lepus_bigint_subtract(difference, high, low));
        if (lepus_bigint_compare(difference, one) <= 0) break;
        CBN_TRY(lepus_bigint_add(middle, low, high));
        CBN_TRY(lepus_bigint_divmod(middle, NULL, middle, two));
        if (square)
            CBN_TRY(lepus_bigint_multiply(power, middle, middle));
        else
            CBN_TRY(lepus_bigint_pow(power, middle, degree));
        if (lepus_bigint_compare(power, value) <= 0)
            CBN_TRY(lepus_bigint_copy(low, middle));
        else
            CBN_TRY(lepus_bigint_copy(high, middle));
    }
    cbn_move(output, low);
cleanup:
    for (index = 0; index < 7; ++index) lepus_bigint_clear(&work[index]);
    return status;
}
/**
 * @brief 使用原倍增上界的整数平方根。
 * @param output 结果。
 * @param value 非负输入。
 * @return 操作状态。
 */
LepusCStatus lepus_bigint_sqrt(LepusBigInt *output, const LepusBigInt *value)
{ return root_integer(output, value, 2, 1); }
/**
 * @brief 使用原位数上界的整数 k 次根。
 * @param output 结果。
 * @param value 非负输入。
 * @param degree 正根次。
 * @return 操作状态。
 */
LepusCStatus lepus_bigint_root(LepusBigInt *output, const LepusBigInt *value, uint64_t degree)
{ return root_integer(output, value, degree, 0); }
#undef CBN_TRY
