/**
 * @file lepus_bigint_division.c
 * @brief 保留 Knuth 分块长除法与 Burnikel-Ziegler 递归的 C 除法。
 */
#include "lepus_bigint_internal.h"
#include <stdint.h>

#define CBN_TRY(call)                        \
    do                                       \
    {                                        \
        status = (call);                     \
        if (status != LEPUS_C_OK) goto done; \
    } while (0)

/**
 * @brief 批量初始化同一所有者的临时数值。
 * @param values 临时对象地址。
 * @param count 对象数。
 * @param owner 分配所有者。
 */
static void initialize_values(LepusBigInt **values, size_t count, const LepusBigInt *owner)
{
    for (size_t index = 0; index < count; ++index) cbn_temporary(values[index], owner);
}
/**
 * @brief 回收全部临时数值。
 * @param values 临时对象地址。
 * @param count 对象数。
 */
static void clear_values(LepusBigInt **values, size_t count)
{
    for (size_t index = 0; index < count; ++index) lepus_bigint_clear(values[index]);
}
/**
 * @brief 复制输入的一段非负分块。
 * @param output 目标。
 * @param value 输入。
 * @param offset 首块位置。
 * @param count 上限。
 * @return 操作状态。
 */
static LepusCStatus limb_slice(LepusBigInt *output, const LepusBigInt *value, size_t offset, size_t count)
{
    if (offset >= value->limbs.length || !count) return lepus_bigint_set_u64(output, 0);
    const size_t remaining = value->limbs.length - offset;
    if (count > remaining) count = remaining;
    LepusBigInt temporary;
    cbn_temporary(&temporary, output);
    LepusCStatus status = lepus_c_buffer_resize(&temporary.limbs, count);
    if (status == LEPUS_C_OK)
    {
        memcpy(cbn_words(&temporary), cbn_const_words(value) + offset, count * sizeof(uint32_t));
        temporary.sign = 1;
        cbn_normalize(&temporary);
        cbn_move(output, &temporary);
    }
    lepus_bigint_clear(&temporary);
    return status;
}
/**
 * @brief 按块合成 high*base^width+low。
 * @param output 目标。
 * @param high 高段。
 * @param low 低段。
 * @param width 低段块宽。
 * @return 操作状态。
 */
static LepusCStatus combine_blocks(LepusBigInt *output, const LepusBigInt *high, const LepusBigInt *low, size_t width)
{
    if (width > SIZE_MAX - high->limbs.length) return LEPUS_C_RANGE_ERROR;
    LepusBigInt temporary;
    cbn_temporary(&temporary, output);
    LepusCStatus status = lepus_c_buffer_resize(&temporary.limbs, width + high->limbs.length);
    if (status == LEPUS_C_OK)
    {
        const size_t count = width < low->limbs.length ? width : low->limbs.length;
        if (count) memcpy(cbn_words(&temporary), cbn_const_words(low), count * sizeof(uint32_t));
        if (high->limbs.length)
            memcpy(cbn_words(&temporary) + width, cbn_const_words(high), high->limbs.length * sizeof(uint32_t));
        temporary.sign = 1;
        cbn_normalize(&temporary);
        cbn_move(output, &temporary);
    }
    lepus_bigint_clear(&temporary);
    return status;
}
/**
 * @brief 对归一化分块估商并校正，保持原 Knuth D 循环。
 * @param quotient 商目标。
 * @param remainder 余数目标。
 * @param left 被除数幅值。
 * @param right 非零除数幅值。
 * @return 操作状态。
 */
static LepusCStatus divide_base(LepusBigInt *quotient, LepusBigInt *remainder, const LepusBigInt *left, const LepusBigInt *right)
{
    LepusBigInt q, r, divisor;
    LepusBigInt *values[] = { &q, &r, &divisor };
    initialize_values(values, 3, quotient);
    LepusCStatus status = LEPUS_C_OK;
    CBN_TRY(lepus_bigint_copy(&r, left));
    if (r.sign) r.sign = 1;
    if (cbn_compare_magnitude(left, right) < 0) goto publish;
    if (right->limbs.length == 1)
    {
        const uint32_t rest = cbn_divide_small(&r, cbn_const_words(right)[0]);
        cbn_move(&q, &r);
        CBN_TRY(lepus_bigint_set_u64(&r, rest));
        goto publish;
    }
    unsigned shift = 0;
    for (uint32_t top = cbn_const_words(right)[right->limbs.length - 1]; !(top & UINT32_C(0x80000000)); top <<= 1) ++shift;
    CBN_TRY(lepus_bigint_copy(&divisor, right));
    divisor.sign = 1;
    CBN_TRY(lepus_bigint_shift_left(&divisor, shift));
    CBN_TRY(lepus_bigint_shift_left(&r, shift));
    const size_t size = divisor.limbs.length;
    const size_t count = left->limbs.length - size + 1;
    CBN_TRY(lepus_c_buffer_resize(&r.limbs, left->limbs.length + 1));
    CBN_TRY(lepus_c_buffer_resize(&q.limbs, count));
    const uint32_t *divisor_data = cbn_const_words(&divisor);
    uint32_t *work = cbn_words(&r);
    const uint64_t base = UINT64_C(1) << 32;
    for (size_t cursor = count; cursor-- > 0;)
    {
        const uint64_t numerator = ((uint64_t)work[cursor + size] << 32) | work[cursor + size - 1];
        uint64_t estimate = numerator / divisor_data[size - 1];
        uint64_t rest = numerator % divisor_data[size - 1];
        while (estimate >= base || (rest < base && estimate * divisor_data[size - 2] > (rest << 32) + work[cursor + size - 2]))
        {
            --estimate;
            rest += divisor_data[size - 1];
            if (rest >= base) break;
        }
        uint64_t borrow = 0;
        for (size_t index = 0; index < size; ++index)
        {
            const uint64_t product = estimate * divisor_data[index] + borrow;
            const uint32_t low = (uint32_t)product;
            borrow = (product >> 32) + (work[cursor + index] < low);
            work[cursor + index] -= low;
        }
        const int negative = work[cursor + size] < borrow;
        work[cursor + size] -= (uint32_t)borrow;
        if (negative)
        {
            --estimate;
            uint64_t carry = 0;
            for (size_t index = 0; index < size; ++index)
            {
                const uint64_t sum = (uint64_t)work[cursor + index] + divisor_data[index] + carry;
                work[cursor + index] = (uint32_t)sum;
                carry = sum >> 32;
            }
            work[cursor + size] += (uint32_t)carry;
        }
        cbn_words(&q)[cursor] = (uint32_t)estimate;
    }
    q.sign = 1;
    cbn_normalize(&q);
    r.limbs.length = size;
    cbn_normalize(&r);
    lepus_bigint_shift_right(&r, shift);
publish:
    CBN_TRY(lepus_bigint_copy(quotient, &q));
    CBN_TRY(lepus_bigint_copy(remainder, &r));
done:
    clear_values(values, 3);
    return status;
}

/**
 * @brief Burnikel-Ziegler 的 2n/1n 子问题。
 * @param quotient 商。
 * @param remainder 余数。
 * @param dividend 被除数。
 * @param divisor 除数。
 * @param width 块宽。
 * @return 操作状态。
 */
static LepusCStatus divide_two_by_one(LepusBigInt *quotient, LepusBigInt *remainder,
                                      const LepusBigInt *dividend, const LepusBigInt *divisor, size_t width);
/**
 * @brief 对正数幅值减一，不分配存储。
 * @param value 正整数。
 */
static void decrement_positive(LepusBigInt *value)
{
    for (size_t index = 0; index < value->limbs.length; ++index)
    {
        const uint32_t previous = cbn_words(value)[index];
        --cbn_words(value)[index];
        if (previous) break;
    }
    cbn_normalize(value);
}
/**
 * @brief Burnikel-Ziegler 的 3n/2n 子问题，保持原估商和负余数修正。
 * @param quotient 商。
 * @param remainder 余数。
 * @param dividend 被除数。
 * @param divisor 除数。
 * @param width 块宽。
 * @return 操作状态。
 */
static LepusCStatus divide_three_by_two(LepusBigInt *quotient, LepusBigInt *remainder,
                                        const LepusBigInt *dividend, const LepusBigInt *divisor, size_t width)
{
    LepusBigInt a3, a2, a1, b2, b1, q, high, r, combined, product;
    LepusBigInt *values[] = { &a3, &a2, &a1, &b2, &b1, &q, &high, &r, &combined, &product };
    initialize_values(values, 10, quotient);
    LepusCStatus status = LEPUS_C_OK;
    CBN_TRY(limb_slice(&a3, dividend, 0, width));
    CBN_TRY(limb_slice(&a2, dividend, width, width));
    CBN_TRY(limb_slice(&a1, dividend, width * 2, width));
    CBN_TRY(limb_slice(&b2, divisor, 0, width));
    CBN_TRY(limb_slice(&b1, divisor, width, width));
    if (cbn_compare_magnitude(&a1, &b1) < 0)
    {
        CBN_TRY(combine_blocks(&combined, &a1, &a2, width));
        CBN_TRY(divide_two_by_one(&q, &high, &combined, &b1, width));
    }
    else
    {
        CBN_TRY(lepus_c_buffer_resize(&q.limbs, width));
        for (size_t index = 0; index < width; ++index) cbn_words(&q)[index] = UINT32_MAX;
        q.sign = 1;
        CBN_TRY(lepus_bigint_subtract(&combined, &a1, &b1));
        CBN_TRY(combine_blocks(&high, &combined, &a2, width));
        CBN_TRY(lepus_bigint_add(&high, &high, &b1));
    }
    CBN_TRY(combine_blocks(&combined, &high, &a3, width));
    CBN_TRY(lepus_bigint_multiply(&product, &q, &b2));
    CBN_TRY(lepus_bigint_subtract(&r, &combined, &product));
    while (r.sign < 0)
    {
        CBN_TRY(lepus_bigint_add(&r, &r, divisor));
        decrement_positive(&q);
    }
    CBN_TRY(lepus_bigint_copy(quotient, &q));
    CBN_TRY(lepus_bigint_copy(remainder, &r));
done:
    clear_values(values, 10);
    return status;
}
/**
 * @brief 将 2n/1n 划为两个 3n/2n 子问题。
 * @param quotient 商。
 * @param remainder 余数。
 * @param dividend 被除数。
 * @param divisor 除数。
 * @param width 块宽。
 * @return 操作状态。
 */
static LepusCStatus divide_two_by_one(LepusBigInt *quotient, LepusBigInt *remainder,
                                      const LepusBigInt *dividend, const LepusBigInt *divisor, size_t width)
{
    if (cbn_compare_magnitude(dividend, divisor) < 0)
    {
        LepusCStatus status = lepus_bigint_set_u64(quotient, 0);
        return status == LEPUS_C_OK ? lepus_bigint_copy(remainder, dividend) : status;
    }
    if (width <= lepus_bigint_division_threshold() || (width & 1))
        return divide_base(quotient, remainder, dividend, divisor);
    const size_t half = width / 2;
    LepusBigInt low, upper, q_high, r_high, q_low, r, combined;
    LepusBigInt *values[] = { &low, &upper, &q_high, &r_high, &q_low, &r, &combined };
    initialize_values(values, 7, quotient);
    LepusCStatus status = LEPUS_C_OK;
    CBN_TRY(limb_slice(&low, dividend, 0, half));
    CBN_TRY(limb_slice(&upper, dividend, half, width + half));
    CBN_TRY(divide_three_by_two(&q_high, &r_high, &upper, divisor, half));
    CBN_TRY(combine_blocks(&combined, &r_high, &low, half));
    CBN_TRY(divide_three_by_two(&q_low, &r, &combined, divisor, half));
    CBN_TRY(combine_blocks(quotient, &q_high, &q_low, half));
    CBN_TRY(lepus_bigint_copy(remainder, &r));
done:
    clear_values(values, 7);
    return status;
}
/**
 * @brief 使用原补齐、最高位归一化和分块扫描执行幅值除法。
 * @param quotient 商。
 * @param remainder 余数。
 * @param dividend 正幅值。
 * @param divisor 非零正幅值。
 * @return 操作状态。
 */
static LepusCStatus divide_magnitude(LepusBigInt *quotient, LepusBigInt *remainder,
                                     const LepusBigInt *dividend, const LepusBigInt *divisor)
{
    if (cbn_compare_magnitude(dividend, divisor) < 0)
    {
        LepusCStatus status = lepus_bigint_set_u64(quotient, 0);
        return status == LEPUS_C_OK ? lepus_bigint_copy(remainder, dividend) : status;
    }
    if (divisor->limbs.length <= lepus_bigint_division_threshold() || dividend->limbs.length - divisor->limbs.length <= 1)
        return divide_base(quotient, remainder, dividend, divisor);
    size_t width = 1;
    while (width < divisor->limbs.length)
    {
        if (width > SIZE_MAX / 2) return LEPUS_C_RANGE_ERROR;
        width <<= 1;
    }
    const size_t padding = width - divisor->limbs.length;
    size_t shift = 0;
    for (uint32_t top = cbn_const_words(divisor)[divisor->limbs.length - 1]; !(top & UINT32_C(0x80000000)); top <<= 1) ++shift;
    if (padding > (SIZE_MAX - shift) / 32) return LEPUS_C_RANGE_ERROR;
    const size_t normalization = padding * 32 + shift;
    LepusBigInt a, b, q, r, chunk, window, q_block;
    LepusBigInt *values[] = { &a, &b, &q, &r, &chunk, &window, &q_block };
    initialize_values(values, 7, quotient);
    LepusCStatus status = LEPUS_C_OK;
    CBN_TRY(lepus_bigint_copy(&a, dividend));
    CBN_TRY(lepus_bigint_copy(&b, divisor));
    CBN_TRY(lepus_bigint_shift_left(&a, normalization));
    CBN_TRY(lepus_bigint_shift_left(&b, normalization));
    const size_t count = a.limbs.length / width + (a.limbs.length % width != 0);
    for (size_t block = count; block-- > 0;)
    {
        CBN_TRY(limb_slice(&chunk, &a, block * width, width));
        CBN_TRY(combine_blocks(&window, &r, &chunk, width));
        CBN_TRY(divide_two_by_one(&q_block, &r, &window, &b, width));
        CBN_TRY(combine_blocks(&q, &q, &q_block, width));
    }
    lepus_bigint_shift_right(&r, normalization);
    cbn_normalize(&q);
    cbn_normalize(&r);
    if (q.limbs.length) q.sign = 1;
    if (r.limbs.length) r.sign = 1;
    CBN_TRY(lepus_bigint_copy(quotient, &q));
    CBN_TRY(lepus_bigint_copy(remainder, &r));
done:
    clear_values(values, 7);
    return status;
}
/**
 * @brief 向零截断的有符号商余，所有临时结果完成后才替换输出。
 * @param quotient 可选商，允许与输入或余数别名，别名时余数最后发布。
 * @param remainder 可选余数。
 * @param dividend 被除数。
 * @param divisor 除数。
 * @return 操作状态。
 */
LepusCStatus lepus_bigint_divmod_work(LepusBigInt *quotient, LepusBigInt *remainder,
                                      const LepusBigInt *dividend, const LepusBigInt *divisor,
                                      const LepusCAllocator *allocator)
{
    if (!dividend || !divisor) return LEPUS_C_INVALID_ARGUMENT;
    if (allocator && !lepus_c_allocator_valid(allocator)) return LEPUS_C_INVALID_ARGUMENT;
    if (!divisor->sign) return LEPUS_C_DOMAIN_ERROR;
    if (!quotient && !remainder) return LEPUS_C_OK;
    const LepusBigInt *owner = quotient ? quotient : remainder;
    LepusBigInt workspace;
    if (allocator)
    {
        lepus_bigint_init(&workspace, allocator);
        owner = &workspace;
    }
    LepusBigInt q, r, q_publish, r_publish;
    cbn_temporary(&q, owner);
    cbn_temporary(&r, owner);
    cbn_temporary(&q_publish, quotient ? quotient : owner);
    cbn_temporary(&r_publish, remainder ? remainder : owner);
    LepusBigInt left_view = *dividend;
    LepusBigInt right_view = *divisor;
    if (left_view.sign) left_view.sign = 1;
    right_view.sign = 1;
    LepusCStatus status = divide_magnitude(&q, &r, &left_view, &right_view);
    if (status != LEPUS_C_OK) goto done;
    if (q.sign) q.sign = dividend->sign * divisor->sign;
    if (r.sign) r.sign = dividend->sign;
    if (quotient) CBN_TRY(lepus_bigint_copy(&q_publish, &q));
    if (remainder) CBN_TRY(lepus_bigint_copy(&r_publish, &r));
    if (quotient) cbn_move(quotient, &q_publish);
    if (remainder) cbn_move(remainder, &r_publish);
done:
    lepus_bigint_clear(&q);
    lepus_bigint_clear(&r);
    lepus_bigint_clear(&q_publish);
    lepus_bigint_clear(&r_publish);
    return status;
}
/**
 * @brief 默认使用结果所有者提供商余临时工作区。
 * @param quotient 可选商。
 * @param remainder 可选余数。
 * @param dividend 被除数。
 * @param divisor 除数。
 * @return 操作状态。
 */
LepusCStatus lepus_bigint_divmod(LepusBigInt *quotient, LepusBigInt *remainder,
                                 const LepusBigInt *dividend, const LepusBigInt *divisor)
{
    return lepus_bigint_divmod_work(quotient, remainder, dividend, divisor, NULL);
}
#undef CBN_TRY
