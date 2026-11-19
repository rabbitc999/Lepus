/**
 * @file lepus_bigfloat.h
 * @brief 以 C 大整数尾数和十进制指数表示的可裁剪大浮点服务。
 */
#ifndef LEPUS_BIGFLOAT_C_H
#define LEPUS_BIGFLOAT_C_H
#include "lepus_bigint.h"
#ifdef __cplusplus
extern "C" {
#endif
/** @brief 独占尾数存储；值为 mantissa × 10^exponent10。 */
typedef struct LepusBigFloat
{
    LepusBigInt mantissa;
    int exponent10;
} LepusBigFloat;
/**
 * @brief 初始化无分配的零。
 * @param value 未持有资源的对象。
 * @param allocator 可选分配接口。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_init(LepusBigFloat *value, const LepusCAllocator *allocator);
/**
 * @brief 释放尾数并重置为零，保留分配接口。
 * @param value 已初始化对象。
 */
void lepus_bigfloat_clear(LepusBigFloat *value);
/**
 * @brief 按目标分配接口复制，允许自复制。
 * @param output 已初始化目标。
 * @param input 输入数值。
 * @return 操作状态；失败不修改目标。
 */
LepusCStatus lepus_bigfloat_copy(LepusBigFloat *output, const LepusBigFloat *input);
/**
 * @brief 从十进制文本解析；语法错误写入零并把 valid 设为零。
 * @param output 已初始化目标。
 * @param text 文本字节，允许前后空格和制表符。
 * @param length 文本字节数。
 * @param valid 成功解析时写入一。
 * @return 分配及范围状态；语法错误本身返回成功。
 */
LepusCStatus lepus_bigfloat_parse(LepusBigFloat *output, const char *text, size_t length, int *valid);
/**
 * @brief 复制整数并规范化。
 * @param output 已初始化目标。
 * @param value 整数输入。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_from_integer(LepusBigFloat *output, const LepusBigInt *value);
/**
 * @brief 使用原有逐精度 snprintf/strtod 往返算法转换 double，非有限值按零处理。
 * @param output 已初始化目标。
 * @param value 双精度输入。
 * @param digits 有效位数上限，范围外按十七位处理。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_from_double(LepusBigFloat *output, double value, int digits);
/**
 * @brief 移除尾数十进制尾随零，失败时保留原值。
 * @param value 已初始化对象。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_normalize(LepusBigFloat *value);
/**
 * @brief 复制并规范化，允许输入与输出相同。
 * @param output 已初始化目标。
 * @param input 输入数值。
 * @return 操作状态；失败保留原目标。
 */
LepusCStatus lepus_bigfloat_normalized(LepusBigFloat *output, const LepusBigFloat *input);
/**
 * @brief 取有效数字，半值远离零舍入。
 * @param output 已初始化目标。
 * @param input 输入数值。
 * @param digits 正有效位数。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_round(LepusBigFloat *output, const LepusBigFloat *input, int digits);
/**
 * @brief 格式化为独占 C 文本。
 * @param output 接受独占文本及其分配接口的对象。
 * @param input 输入数值。
 * @param digits 有效数字，非正时保留精确值。
 * @param plain 非零时恒用普通记法。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_to_string(LepusBigString *output, const LepusBigFloat *input, int digits, int plain);
/**
 * @brief 比较两个十进制数，沿用对齐指数再比较大整数的算法。
 * @param left 左输入。
 * @param right 右输入。
 * @param order 写入负一、零或一。
 * @param allocator 临时工作区分配接口。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_compare(const LepusBigFloat *left, const LepusBigFloat *right, int *order,
                                    const LepusCAllocator *allocator);
/**
 * @brief 精确加减法，所有输入允许与输出别名。
 * @param output 已初始化目标。
 * @param left 左输入。
 * @param right 右输入。
 * @param subtract 非零时计算差。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_add(LepusBigFloat *output, const LepusBigFloat *left, const LepusBigFloat *right, int subtract);
/**
 * @brief 通过原大整数乘法内核计算精确乘积。
 * @param output 已初始化目标。
 * @param left 左输入。
 * @param right 右输入。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_multiply(LepusBigFloat *output, const LepusBigFloat *left, const LepusBigFloat *right);
/**
 * @brief 沿用放大被除数、整数除法和保护位舍入算法。
 * @param output 已初始化目标。
 * @param left 被除数。
 * @param right 除数。
 * @param digits 正有效位数。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_divide(LepusBigFloat *output, const LepusBigFloat *left, const LepusBigFloat *right, int digits);
/**
 * @brief 复制并改变符号或取绝对值。
 * @param output 已初始化目标。
 * @param input 输入数值。
 * @param absolute 非零取绝对值，否则取负数。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_sign_change(LepusBigFloat *output, const LepusBigFloat *input, int absolute);
/**
 * @brief 用整数平方根计算十进制平方根并舍入。
 * @param output 已初始化目标。
 * @param input 非负输入。
 * @param digits 正有效位数。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_sqrt(LepusBigFloat *output, const LepusBigFloat *input, int digits);
/**
 * @brief 精确计算非负整数次幂。
 * @param output 已初始化目标。
 * @param input 输入数值。
 * @param exponent 非负指数。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_pow_integer(LepusBigFloat *output, const LepusBigFloat *input, uint64_t exponent);
/**
 * @brief 保留原有预热及精度翻倍的牛顿迭代计算方根。
 * @param output 已初始化目标。
 * @param input 非负输入。
 * @param degree 正根次。
 * @param digits 正有效位数。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_root(LepusBigFloat *output, const LepusBigFloat *input, uint64_t degree, int digits);
/**
 * @brief 检查数值是否为整数。
 * @param input 输入数值。
 * @param integer 写入布尔结果。
 * @param allocator 工作区分配接口。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_is_integer(const LepusBigFloat *input, int *integer, const LepusCAllocator *allocator);
/**
 * @brief 转为整数，可选择向零、向下或向上取整。
 * @param output 已初始化整数目标。
 * @param input 输入数值。
 * @param mode 零表示截断，负值向下，正值向上。
 * @return 操作状态。
 */
LepusCStatus lepus_bigfloat_to_integer(LepusBigInt *output, const LepusBigFloat *input, int mode);
#ifdef __cplusplus
}
#endif
#endif
