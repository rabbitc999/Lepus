/**
 * @file lepus_bigfloat_math.h
 * @brief 保留既有保护位和级数算法的 C 高精度数学服务。
 */
#ifndef LEPUS_BIGFLOAT_MATH_C_H
#define LEPUS_BIGFLOAT_MATH_C_H
#include "lepus_bigfloat.h"
#ifdef __cplusplus
extern "C" {
#endif
/** @brief 不分配文本的错误类别，用于恢复既有 C++ 诊断协议。 */
typedef enum LepusBigFloatErrorKind
{
    LEPUS_BIGFLOAT_ERROR_NONE = 0,
    LEPUS_BIGFLOAT_ERROR_PRECISION = 1,
    LEPUS_BIGFLOAT_ERROR_OVERFLOW = 2,
    LEPUS_BIGFLOAT_ERROR_DOMAIN = 3,
    LEPUS_BIGFLOAT_ERROR_CONVERGENCE = 4
} LepusBigFloatErrorKind;
/** @brief 错误消息借用静态常量，不包含线程全局状态或动态所有权。 */
typedef struct LepusBigFloatError
{
    LepusBigFloatErrorKind kind;
    const char *message;
} LepusBigFloatError;
/**
 * @brief 检查工作精度并加入原有三十二位保护位。
 * @param digits 请求有效位数。
 * @param extra 参数约化所需额外位数。
 * @param precision 写入工作位数。
 * @param error 可选错误细节。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_work_precision(int digits, int64_t extra, int *precision, LepusBigFloatError *error);
/**
 * @brief 取得最高有效十进制位的指数。
 * @param value 输入数值。
 * @param order 指数输出，零输入返回零。
 * @param allocator 工作分配接口。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_decimal_order(const LepusBigFloat *value, int64_t *order, const LepusCAllocator *allocator);
/**
 * @brief 通过 Gauss-Legendre AGM 计算圆周率。
 * @param output 已初始化目标。
 * @param digits 结果有效位数。
 * @param error 可选错误细节。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_pi(LepusBigFloat *output, int digits, LepusBigFloatError *error);
/**
 * @brief 约化参数并通过 Taylor 级数及重复平方求自然指数。
 * @param output 已初始化目标。
 * @param input 实数输入。
 * @param digits 有效位数。
 * @param error 可选错误细节。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_exp(LepusBigFloat *output, const LepusBigFloat *input, int digits, LepusBigFloatError *error);
/**
 * @brief 通过数量级分离和反双曲正切级数计算自然对数。
 * @param output 已初始化目标。
 * @param input 正输入。
 * @param digits 有效位数。
 * @param error 可选错误细节。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_ln(LepusBigFloat *output, const LepusBigFloat *input, int digits, LepusBigFloatError *error);
/**
 * @brief 计算常用对数，十的整数幂直接返回指数。
 * @param output 已初始化目标。
 * @param input 正输入。
 * @param digits 有效位数。
 * @param error 可选错误细节。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_log10(LepusBigFloat *output, const LepusBigFloat *input, int digits, LepusBigFloatError *error);
/**
 * @brief 用定精度整数幂或指数/对数实现实数乘方。
 * @param output 已初始化目标。
 * @param input 底数。
 * @param exponent 实数指数。
 * @param digits 有效位数。
 * @param error 可选错误细节。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_pow(LepusBigFloat *output, const LepusBigFloat *input, const LepusBigFloat *exponent,
                                int digits, LepusBigFloatError *error);
/**
 * @brief 按整数模算法精确计算十进制余数。
 * @param output 已初始化目标。
 * @param input 被除数。
 * @param divisor 非零除数。
 * @param error 可选错误细节。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_fmod(LepusBigFloat *output, const LepusBigFloat *input, const LepusBigFloat *divisor,
                                 LepusBigFloatError *error);
/**
 * @brief 计算弧度输入的高精度正弦。
 * @param output 已初始化目标。
 * @param input 实数输入。
 * @param digits 有效位数。
 * @param error 可选错误细节。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_sin(LepusBigFloat *output, const LepusBigFloat *input, int digits, LepusBigFloatError *error);
/**
 * @brief 计算弧度输入的高精度余弦。
 * @param output 已初始化目标。
 * @param input 实数输入。
 * @param digits 有效位数。
 * @param error 可选错误细节。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_cos(LepusBigFloat *output, const LepusBigFloat *input, int digits, LepusBigFloatError *error);
/**
 * @brief 复用同一次角度约化计算高精度正切。
 * @param output 已初始化目标。
 * @param input 实数输入。
 * @param digits 有效位数。
 * @param error 可选错误细节。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_tan(LepusBigFloat *output, const LepusBigFloat *input, int digits, LepusBigFloatError *error);
/**
 * @brief 通过倒数、半角约化和交错级数计算反正切。
 * @param output 已初始化目标。
 * @param input 实数输入。
 * @param digits 有效位数。
 * @param error 可选错误细节。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_atan(LepusBigFloat *output, const LepusBigFloat *input, int digits, LepusBigFloatError *error);
/**
 * @brief 通过平方根和反正切计算反正弦主值。
 * @param output 已初始化目标。
 * @param input 实数输入。
 * @param digits 有效位数。
 * @param error 可选错误细节。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_asin(LepusBigFloat *output, const LepusBigFloat *input, int digits, LepusBigFloatError *error);
/**
 * @brief 通过半角恒等式计算反余弦主值。
 * @param output 已初始化目标。
 * @param input 实数输入。
 * @param digits 有效位数。
 * @param error 可选错误细节。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_acos(LepusBigFloat *output, const LepusBigFloat *input, int digits, LepusBigFloatError *error);
#ifdef __cplusplus
}
#endif
#endif
