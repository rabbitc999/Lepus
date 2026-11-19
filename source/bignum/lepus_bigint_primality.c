/**
 * @file lepus_bigint_primality.c
 * @brief 保持固定底数序列的 Miller-Rabin C 实现。
 */
#include "lepus_bigint_internal.h"
#define CBN_TRY(expression)                     \
    do                                          \
    {                                           \
        status = (expression);                  \
        if (status != LEPUS_C_OK) goto cleanup; \
    } while (0)
/**
 * @brief 按原小素数试除、固定 xorshift64 底数及 Miller-Rabin 次序判定素性。
 * @param value 待测整数。
 * @param rounds 检验轮数，零轮只试除。
 * @param probable 接收判定，失败保持原值。
 * @param allocator 可选临时工作区所有者。
 * @return 操作状态。
 */
LepusCStatus lepus_bigint_is_probable_prime(const LepusBigInt *value, size_t rounds, int *probable,
                                            const LepusCAllocator *allocator)
{
    static const uint32_t primes[] = { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37 };
    LepusBigInt work[9];
    LepusBigInt *one = &work[0], *two = &work[1], *prime = &work[2], *remainder = &work[3];
    LepusBigInt *d = &work[4], *minus_one = &work[5], *minus_three = &work[6];
    LepusBigInt *base = &work[7], *x = &work[8];
    uint64_t random_state = UINT64_C(88172645463325252);
    size_t index, shifts = 0, round;
    int result = 0;
    LepusCStatus status = LEPUS_C_OK;
    if (!value || !probable || (allocator && !lepus_c_allocator_valid(allocator))) return LEPUS_C_INVALID_ARGUMENT;
    if (value->sign <= 0 || (value->limbs.length == 1 && cbn_const_words(value)[0] < 2))
    {
        *probable = 0;
        return LEPUS_C_OK;
    }
    for (index = 0; index < 9; ++index)
        lepus_bigint_init(&work[index], allocator ? allocator : &value->limbs.allocator);
    for (index = 0; index < sizeof(primes) / sizeof(primes[0]); ++index)
    {
        CBN_TRY(lepus_bigint_set_u64(prime, primes[index]));
        CBN_TRY(lepus_bigint_divmod(NULL, remainder, value, prime));
        if (!remainder->sign)
        {
            result = lepus_bigint_compare(value, prime) == 0;
            goto publish;
        }
    }
    if (value->limbs.length == 1 && cbn_const_words(value)[0] < 1681)
    {
        result = 1;
        goto publish;
    }
    CBN_TRY(lepus_bigint_set_u64(one, 1));
    CBN_TRY(lepus_bigint_set_u64(two, 2));
    CBN_TRY(lepus_bigint_subtract(minus_one, value, one));
    CBN_TRY(lepus_bigint_set_u64(prime, 3));
    CBN_TRY(lepus_bigint_subtract(minus_three, value, prime));
    CBN_TRY(lepus_bigint_copy(d, minus_one));
    while (!(cbn_const_words(d)[0] & 1u))
    {
        lepus_bigint_shift_right(d, 1);
        ++shifts;
    }
    for (round = 0; round < rounds; ++round)
    {
        size_t step;
        int witness = 1;
        random_state ^= random_state << 13;
        random_state ^= random_state >> 7;
        random_state ^= random_state << 17;
        CBN_TRY(lepus_bigint_set_u64(base, random_state));
        CBN_TRY(lepus_bigint_mod(base, base, minus_three));
        CBN_TRY(lepus_bigint_add(base, base, two));
        CBN_TRY(lepus_bigint_powmod(x, base, d, value));
        if (lepus_bigint_compare(x, one) == 0 || lepus_bigint_compare(x, minus_one) == 0) continue;
        for (step = 1; step < shifts; ++step)
        {
            CBN_TRY(lepus_bigint_multiply(x, x, x));
            CBN_TRY(lepus_bigint_divmod(NULL, x, x, value));
            if (lepus_bigint_compare(x, minus_one) == 0)
            {
                witness = 0;
                break;
            }
        }
        if (witness) goto publish;
    }
    result = 1;
publish:
    *probable = result;
cleanup:
    for (index = 0; index < 9; ++index) lepus_bigint_clear(&work[index]);
    return status;
}
/**
 * @brief 按原逐奇数递增算法寻找严格更大的概率素数。
 * @param output 结果。
 * @param value 起点。
 * @param rounds 检验轮数。
 * @return 操作状态，失败保持目标不变。
 */
LepusCStatus lepus_bigint_next_probable_prime(LepusBigInt *output, const LepusBigInt *value, size_t rounds)
{
    LepusBigInt candidate, one, two;
    LepusCStatus status = LEPUS_C_OK;
    int probable = 0;
    if (!output || !value) return LEPUS_C_INVALID_ARGUMENT;
    if (value->sign <= 0 || (value->limbs.length == 1 && cbn_const_words(value)[0] < 2))
        return lepus_bigint_set_u64(output, 2);
    cbn_temporary(&candidate, output);
    cbn_temporary(&one, output);
    cbn_temporary(&two, output);
    CBN_TRY(lepus_bigint_set_u64(&one, 1));
    CBN_TRY(lepus_bigint_set_u64(&two, 2));
    CBN_TRY(lepus_bigint_add(&candidate, value, &one));
    if (!(cbn_const_words(&candidate)[0] & 1u)) CBN_TRY(lepus_bigint_add(&candidate, &candidate, &one));
    for (;;)
    {
        CBN_TRY(lepus_bigint_is_probable_prime(&candidate, rounds, &probable, &output->limbs.allocator));
        if (probable) break;
        CBN_TRY(lepus_bigint_add(&candidate, &candidate, &two));
    }
    cbn_move(output, &candidate);
cleanup:
    lepus_bigint_clear(&candidate);
    lepus_bigint_clear(&one);
    lepus_bigint_clear(&two);
    return status;
}
#undef CBN_TRY
