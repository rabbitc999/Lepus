/**
 * @file abi_math.c
 * @brief `std::math` 的直通 C 实现。
 *
 * 本模块的入口只做标量运算，不访问堆与对象表，因此可以在直通路径上直接完成，
 * 不需要经过 `NativeValue` 编解码与 `Memory::Function` 间接调用。
 *
 * 参数槽的值类别由生成代码给出：浮点入口要求调用点写入的是双精度位模式，
 * 与四元式 `float64` 实参一致。
 */
#include "lepus_abi_internal.h"

#include <math.h>

/**
 * @brief 生成一个单目双精度直通入口。
 * @param functionName 生成的入口函数名。
 * @param expression 以 `x` 为自变量的返回表达式。
 */
#define LEPUS_ABI_UNARY_REAL(functionName, expression)      \
    static void functionName(LepusAbiFrame *frame)          \
    {                                                       \
        double x = 0.0;                                     \
        if (!lepus_abi_argument_real(frame, 0, &x)) return; \
        lepus_abi_return_real(frame, (expression));         \
    }

/**
 * @brief 生成一个双目双精度直通入口。
 * @param functionName 生成的入口函数名。
 * @param expression 以 `x`、`y` 为操作数的返回表达式。
 */
#define LEPUS_ABI_BINARY_REAL(functionName, expression)     \
    static void functionName(LepusAbiFrame *frame)          \
    {                                                       \
        double x = 0.0, y = 0.0;                            \
        if (!lepus_abi_argument_real(frame, 0, &x)) return; \
        if (!lepus_abi_argument_real(frame, 1, &y)) return; \
        lepus_abi_return_real(frame, (expression));         \
    }

LEPUS_ABI_UNARY_REAL(abiMathSin, sin(x))
LEPUS_ABI_UNARY_REAL(abiMathCos, cos(x))
LEPUS_ABI_UNARY_REAL(abiMathTan, tan(x))
LEPUS_ABI_UNARY_REAL(abiMathAsin, asin(x))
LEPUS_ABI_UNARY_REAL(abiMathAcos, acos(x))
LEPUS_ABI_UNARY_REAL(abiMathAtan, atan(x))
LEPUS_ABI_UNARY_REAL(abiMathExp, exp(x))
LEPUS_ABI_UNARY_REAL(abiMathLn, log(x))
LEPUS_ABI_UNARY_REAL(abiMathLog10, log10(x))
LEPUS_ABI_UNARY_REAL(abiMathSqrt, sqrt(x))
LEPUS_ABI_UNARY_REAL(abiMathFloor, floor(x))
LEPUS_ABI_UNARY_REAL(abiMathCeil, ceil(x))
LEPUS_ABI_UNARY_REAL(abiMathRound, round(x))
LEPUS_ABI_UNARY_REAL(abiMathTrunc, trunc(x))

LEPUS_ABI_BINARY_REAL(abiMathPow, pow(x, y))
LEPUS_ABI_BINARY_REAL(abiMathFmod, fmod(x, y))

/**
 * @brief 非数判定。
 * @param frame 帧：0 = 待判定值。
 */
static void abiMathIsNan(LepusAbiFrame *frame)
{
    double x = 0.0;
    if (!lepus_abi_argument_real(frame, 0, &x)) return;
    lepus_abi_return_integer(frame, isnan(x) ? 1 : 0);
}

/**
 * @brief 无穷判定。
 * @param frame 帧：0 = 待判定值。
 */
static void abiMathIsInf(LepusAbiFrame *frame)
{
    double x = 0.0;
    if (!lepus_abi_argument_real(frame, 0, &x)) return;
    lepus_abi_return_integer(frame, isinf(x) ? 1 : 0);
}

/**
 * @brief 本模块入口表，按名称升序排列。
 */
static const LepusAbiRecord kRecords[] = {
    { "std::math::__acos64", abiMathAcos, 1 },
    { "std::math::__asin64", abiMathAsin, 1 },
    { "std::math::__atan64", abiMathAtan, 1 },
    { "std::math::__ceil64", abiMathCeil, 1 },
    { "std::math::__cos64", abiMathCos, 1 },
    { "std::math::__exp64", abiMathExp, 1 },
    { "std::math::__floor64", abiMathFloor, 1 },
    { "std::math::__fmod64", abiMathFmod, 2 },
    { "std::math::__isInf64", abiMathIsInf, 1 },
    { "std::math::__isNaN64", abiMathIsNan, 1 },
    { "std::math::__ln64", abiMathLn, 1 },
    { "std::math::__log1064", abiMathLog10, 1 },
    { "std::math::__pow64", abiMathPow, 2 },
    { "std::math::__round64", abiMathRound, 1 },
    { "std::math::__sin64", abiMathSin, 1 },
    { "std::math::__sqrt64", abiMathSqrt, 1 },
    { "std::math::__tan64", abiMathTan, 1 },
    { "std::math::__trunc64", abiMathTrunc, 1 },
    { "std::math::acos", abiMathAcos, 1 },
    { "std::math::asin", abiMathAsin, 1 },
    { "std::math::atan", abiMathAtan, 1 },
    { "std::math::ceil", abiMathCeil, 1 },
    { "std::math::cos", abiMathCos, 1 },
    { "std::math::exp", abiMathExp, 1 },
    { "std::math::floor", abiMathFloor, 1 },
    { "std::math::fmod", abiMathFmod, 2 },
    { "std::math::isInf", abiMathIsInf, 1 },
    { "std::math::isNaN", abiMathIsNan, 1 },
    { "std::math::ln", abiMathLn, 1 },
    { "std::math::log10", abiMathLog10, 1 },
    { "std::math::pow", abiMathPow, 2 },
    { "std::math::round", abiMathRound, 1 },
    { "std::math::sin", abiMathSin, 1 },
    { "std::math::sqrt", abiMathSqrt, 1 },
    { "std::math::tan", abiMathTan, 1 },
    { "std::math::trunc", abiMathTrunc, 1 },
};

const LepusAbiRecord *abi_math_records(size_t *count)
{
    if (count != NULL) *count = sizeof(kRecords) / sizeof(kRecords[0]);
    return kRecords;
}

#ifndef GLUE_NO_STD_MATH
#define LEPUS_BUILD_BIND_MODULE math
#include "aot_bind_module.h"
#endif
