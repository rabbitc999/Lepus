/**
 * @file lepus_bigfloat_arithmetic.c
 * @brief 十进制 C 四则、有效位舍入和比较。
 */
#include "lepus_bigfloat_internal.h"

/**
 * @brief 对齐一个尾数到更小的十进制指数。
 * @param output 已初始化目标整数。
 * @param value 原浮点数。
 * @param exponent 目标指数。
 * @return 操作状态。
 */
static LepusCStatus cbf_aligned(LepusBigInt *output, const LepusBigFloat *value, int exponent)
{
    LepusBigInt scale;
    LepusCStatus status;
    cbn_temporary(&scale, output);
    status = cbf_pow10(&scale, (uint64_t)((int64_t)value->exponent10 - exponent));
    if (status == LEPUS_C_OK) status = lepus_bigint_multiply(output, &value->mantissa, &scale);
    lepus_bigint_clear(&scale);
    return status;
}
/**
 * @brief 对两个数对齐指数后计算精确和或差。
 * @param output 已初始化目标。
 * @param left 左输入。
 * @param right 右输入。
 * @param subtract 非零计算差。
 * @return 操作状态，失败不改变目标。
 */
LepusCStatus lepus_bigfloat_add(LepusBigFloat *output, const LepusBigFloat *left, const LepusBigFloat *right, int subtract)
{
    LepusBigFloat work;
    LepusBigInt a, b;
    LepusCStatus status;
    int exponent;
    if (!output || !left || !right) return LEPUS_C_INVALID_ARGUMENT;
    if (!right->mantissa.sign) return lepus_bigfloat_copy(output, left);
    if (!left->mantissa.sign)
        return subtract ? lepus_bigfloat_sign_change(output, right, 0) : lepus_bigfloat_copy(output, right);
    exponent = left->exponent10 < right->exponent10 ? left->exponent10 : right->exponent10;
    lepus_bigfloat_init(&work, &output->mantissa.limbs.allocator);
    cbn_temporary(&a, &output->mantissa);
    cbn_temporary(&b, &output->mantissa);
    CBF_TRY(cbf_aligned(&a, left, exponent));
    CBF_TRY(cbf_aligned(&b, right, exponent));
    if (subtract) b.sign = -b.sign;
    CBF_TRY(lepus_bigint_add(&work.mantissa, &a, &b));
    work.exponent10 = exponent;
    CBF_TRY(cbf_normalize_work(&work));
    cbf_move(output, &work);
cleanup:
    lepus_bigint_clear(&a);
    lepus_bigint_clear(&b);
    lepus_bigfloat_clear(&work);
    return status;
}
/**
 * @brief 按原大整数乘法和指数相加算法计算积。
 * @param output 已初始化目标。
 * @param left 左输入。
 * @param right 右输入。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_multiply(LepusBigFloat *output, const LepusBigFloat *left, const LepusBigFloat *right)
{
    LepusBigFloat work;
    LepusCStatus status;
    int64_t exponent;
    if (!output || !left || !right) return LEPUS_C_INVALID_ARGUMENT;
    exponent = (int64_t)left->exponent10 + right->exponent10;
    if (exponent < INT_MIN || exponent > INT_MAX) return LEPUS_C_RANGE_ERROR;
    lepus_bigfloat_init(&work, &output->mantissa.limbs.allocator);
    CBF_TRY(lepus_bigint_multiply(&work.mantissa, &left->mantissa, &right->mantissa));
    work.exponent10 = (int)exponent;
    CBF_TRY(cbf_normalize_work(&work));
    cbf_move(output, &work);
cleanup:
    lepus_bigfloat_clear(&work);
    return status;
}
/**
 * @brief 复制后取负数或绝对值。
 * @param output 已初始化目标。
 * @param input 原值。
 * @param absolute 非零取绝对值。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_sign_change(LepusBigFloat *output, const LepusBigFloat *input, int absolute)
{
    LepusCStatus status = lepus_bigfloat_copy(output, input);
    if (status == LEPUS_C_OK)
    {
        if (absolute)
        {
            if (output->mantissa.sign < 0) output->mantissa.sign = -output->mantissa.sign;
        }
        else
            output->mantissa.sign = -output->mantissa.sign;
    }
    return status;
}
/**
 * @brief 除去多余十进制位，半值向远离零方向舍入。
 * @param output 已初始化目标。
 * @param input 原值。
 * @param digits 需要的正有效位数。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_round(LepusBigFloat *output, const LepusBigFloat *input, int digits)
{
    LepusBigFloat work;
    LepusBigInt scale, remainder, twice, unit;
    LepusCStatus status;
    int64_t have, shift, exponent;
    if (!output || !input || digits <= 0) return LEPUS_C_INVALID_ARGUMENT;
    lepus_bigfloat_init(&work, &output->mantissa.limbs.allocator);
    cbn_temporary(&scale, &output->mantissa);
    cbn_temporary(&remainder, &output->mantissa);
    cbn_temporary(&twice, &output->mantissa);
    cbn_temporary(&unit, &output->mantissa);
    CBF_TRY(cbf_digits(&input->mantissa, &have, &output->mantissa.limbs.allocator));
    if (!input->mantissa.sign || have <= digits)
    {
        CBF_TRY(lepus_bigfloat_copy(&work, input));
        goto commit;
    }
    shift = have - digits;
    exponent = (int64_t)input->exponent10 + shift;
    if (exponent < INT_MIN || exponent > INT_MAX)
    {
        status = LEPUS_C_RANGE_ERROR;
        goto cleanup;
    }
    CBF_TRY(cbf_pow10(&scale, (uint64_t)shift));
    CBF_TRY(lepus_bigint_divmod_work(&work.mantissa, &remainder, &input->mantissa, &scale,
                                     &output->mantissa.limbs.allocator));
    if (remainder.sign < 0) remainder.sign = -remainder.sign;
    CBF_TRY(lepus_bigint_set_u64(&unit, 2));
    CBF_TRY(lepus_bigint_multiply(&twice, &remainder, &unit));
    if (lepus_bigint_compare(&twice, &scale) >= 0)
    {
        CBF_TRY(lepus_bigint_set_i64(&unit, input->mantissa.sign < 0 ? -1 : 1));
        CBF_TRY(lepus_bigint_add(&work.mantissa, &work.mantissa, &unit));
    }
    work.exponent10 = (int)exponent;
    CBF_TRY(cbf_normalize_work(&work));
commit:
    cbf_move(output, &work);
cleanup:
    lepus_bigfloat_clear(&work);
    lepus_bigint_clear(&scale);
    lepus_bigint_clear(&remainder);
    lepus_bigint_clear(&twice);
    lepus_bigint_clear(&unit);
    return status;
}
/**
 * @brief 放大被除数后用整数除法计算，再按请求位数舍入。
 * @param output 已初始化目标。
 * @param left 被除数。
 * @param right 非零除数。
 * @param digits 正有效位数。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_divide(LepusBigFloat *output, const LepusBigFloat *left, const LepusBigFloat *right, int digits)
{
    LepusBigFloat work;
    LepusBigInt numerator, scale;
    LepusCStatus status;
    int64_t numerator_digits, denominator_digits, shift, exponent;
    if (!output || !left || !right) return LEPUS_C_INVALID_ARGUMENT;
    if (!right->mantissa.sign) return LEPUS_C_DOMAIN_ERROR;
    if (digits <= 0) return LEPUS_C_INVALID_ARGUMENT;
    lepus_bigfloat_init(&work, &output->mantissa.limbs.allocator);
    cbn_temporary(&numerator, &output->mantissa);
    cbn_temporary(&scale, &output->mantissa);
    if (!left->mantissa.sign) goto commit;
    CBF_TRY(cbf_digits(&left->mantissa, &numerator_digits, &output->mantissa.limbs.allocator));
    CBF_TRY(cbf_digits(&right->mantissa, &denominator_digits, &output->mantissa.limbs.allocator));
    shift = (int64_t)digits + 2 + denominator_digits - numerator_digits;
    exponent = (int64_t)left->exponent10 - right->exponent10;
    CBF_TRY(lepus_bigint_copy(&numerator, &left->mantissa));
    if (shift > 0)
    {
        CBF_TRY(cbf_pow10(&scale, (uint64_t)shift));
        CBF_TRY(lepus_bigint_multiply(&numerator, &numerator, &scale));
        exponent -= shift;
    }
    if (exponent < INT_MIN || exponent > INT_MAX)
    {
        status = LEPUS_C_RANGE_ERROR;
        goto cleanup;
    }
    CBF_TRY(lepus_bigint_divmod_work(&work.mantissa, NULL, &numerator, &right->mantissa,
                                     &output->mantissa.limbs.allocator));
    work.exponent10 = (int)exponent;
    CBF_TRY(cbf_normalize_work(&work));
    CBF_TRY(lepus_bigfloat_round(&work, &work, digits));
commit:
    status = LEPUS_C_OK;
    cbf_move(output, &work);
cleanup:
    lepus_bigfloat_clear(&work);
    lepus_bigint_clear(&numerator);
    lepus_bigint_clear(&scale);
    return status;
}
/**
 * @brief 对齐同号输入的指数后比较幅值。
 * @param left 左输入。
 * @param right 右输入。
 * @param order 比较结果输出。
 * @param allocator 工作区分配接口。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_compare(const LepusBigFloat *left, const LepusBigFloat *right, int *order,
                                    const LepusCAllocator *allocator)
{
    LepusBigInt a, b;
    LepusCStatus status = LEPUS_C_OK;
    int exponent, comparison;
    if (!left || !right || !order) return LEPUS_C_INVALID_ARGUMENT;
    if (left->mantissa.sign != right->mantissa.sign)
    {
        *order = left->mantissa.sign < right->mantissa.sign ? -1 : 1;
        return LEPUS_C_OK;
    }
    if (!left->mantissa.sign)
    {
        *order = 0;
        return LEPUS_C_OK;
    }
    status = lepus_bigint_init(&a, allocator);
    if (status != LEPUS_C_OK) return status;
    lepus_bigint_init(&b, &a.limbs.allocator);
    exponent = left->exponent10 < right->exponent10 ? left->exponent10 : right->exponent10;
    CBF_TRY(cbf_aligned(&a, left, exponent));
    CBF_TRY(cbf_aligned(&b, right, exponent));
    comparison = lepus_bigint_compare(&a, &b);
    *order = comparison;
cleanup:
    lepus_bigint_clear(&a);
    lepus_bigint_clear(&b);
    return status;
}
