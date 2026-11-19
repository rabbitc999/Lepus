/**
 * @file lepus_bigint.h
 * @brief 任意精度整数的 C 接口，使用 32 位小端分块与调用方分配器。
 */
#ifndef LEPUS_BIGINT_C_H
#define LEPUS_BIGINT_C_H
#include "../runtime_c/lepus_buffer.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/** @brief 独占的有符号整数，零的符号与有效分块数均为零。 */
typedef struct LepusBigInt
{
    int sign;
    LepusCBuffer limbs;
} LepusBigInt;
/** @brief 独占的 UTF-8 数值文本。 */
typedef struct LepusBigString
{
    char *data;
    size_t length;
    size_t capacity;
    LepusCAllocator allocator;
} LepusBigString;
/**
 * @brief 建立数值为零的整数。
 * @param value 未持有存储的目标。
 * @param allocator 可选分配器。
 * @return 操作状态。
 */
LepusCStatus lepus_bigint_init(LepusBigInt *value, const LepusCAllocator *allocator);
/**
 * @brief 回收分块并保留分配接口。
 * @param value 已初始化整数。
 */
void lepus_bigint_clear(LepusBigInt *value);
/**
 * @brief 释放转换得到的文本。
 * @param text 独占文本。
 */
void lepus_big_string_clear(LepusBigString *text);
/**
 * @brief 复制数值，失败时不修改目标。
 * @param output 已初始化目标。
 * @param input 输入数值。
 * @return 操作状态。
 */
LepusCStatus lepus_bigint_copy(LepusBigInt *output, const LepusBigInt *input);
/**
 * @brief 写入无符号 64 位整数。
 * @param output 已初始化目标。
 * @param value 数值。
 * @return 操作状态。
 */
LepusCStatus lepus_bigint_set_u64(LepusBigInt *output, uint64_t value);
/**
 * @brief 写入有符号 64 位整数，支持最小负数。
 * @param output 已初始化目标。
 * @param value 数值。
 * @return 操作状态。
 */
LepusCStatus lepus_bigint_set_i64(LepusBigInt *output, int64_t value);
/**
 * @brief 比较有符号数值。
 * @param left 左值。
 * @param right 右值。
 * @return 负一、零或一。
 */
int lepus_bigint_compare(const LepusBigInt *left, const LepusBigInt *right);
/**
 * @brief 加法，允许输出与任一输入为同一对象。
 * @param output 目标。
 * @param left 左值。
 * @param right 右值。
 * @return 操作状态。
 */
LepusCStatus lepus_bigint_add(LepusBigInt *output, const LepusBigInt *left, const LepusBigInt *right);
/**
 * @brief 减法，允许输出与输入别名。
 * @param output 目标。
 * @param left 左值。
 * @param right 右值。
 * @return 操作状态。
 */
LepusCStatus lepus_bigint_subtract(LepusBigInt *output, const LepusBigInt *left, const LepusBigInt *right);
/**
 * @brief 使用原有 Karatsuba 或双素数 NTT+CRT 分派计算乘积。
 * @param output 目标。
 * @param left 左值。
 * @param right 右值。
 * @return 操作状态。
 */
LepusCStatus lepus_bigint_multiply(LepusBigInt *output, const LepusBigInt *left, const LepusBigInt *right);
/**
 * @brief 向零截断的商余，保持原有 Burnikel-Ziegler 与 Knuth 分派。
 * @param quotient 可选商目标。
 * @param remainder 可选余数目标，与商别名时最后接收余数。
 * @param dividend 被除数。
 * @param divisor 除数。
 * @return 操作状态，除零为定义域错误。
 */
LepusCStatus lepus_bigint_divmod(LepusBigInt *quotient, LepusBigInt *remainder,
                                 const LepusBigInt *dividend, const LepusBigInt *divisor);
/**
 * @brief 商余运算的工作区与最终结果可使用不同所有者。
 * @param quotient 可选商。
 * @param remainder 可选余数。
 * @param dividend 被除数。
 * @param divisor 除数。
 * @param allocator 可选工作区所有者，空值沿用结果所有者。
 * @return 操作状态，结果全部成功后才发布。
 */
LepusCStatus lepus_bigint_divmod_work(LepusBigInt *quotient, LepusBigInt *remainder,
                                      const LepusBigInt *dividend, const LepusBigInt *divisor,
                                      const LepusCAllocator *allocator);
/**
 * @brief 读取幅值的低 64 位，忽略符号。
 * @param value 数值。
 * @return 无符号位模式。
 */
uint64_t lepus_bigint_to_u64(const LepusBigInt *value);
/**
 * @brief 读取有效二进制位数。
 * @param value 数值。
 * @return 幅值位数，零返回零。
 */
size_t lepus_bigint_bit_length(const LepusBigInt *value);
/**
 * @brief 原地左移幅值并保留符号。
 * @param value 数值。
 * @param bits 左移位数。
 * @return 操作状态。
 */
LepusCStatus lepus_bigint_shift_left(LepusBigInt *value, size_t bits);
/**
 * @brief 原地右移幅值并保留符号。
 * @param value 数值。
 * @param bits 右移位数。
 */
void lepus_bigint_shift_right(LepusBigInt *value, size_t bits);
/**
 * @brief 解析整数，base 为零时使用原严格十进制入口。
 * @param output 目标。
 * @param text 文本字节。
 * @param size 字节数。
 * @param base 零或二至三十六。
 * @return 操作状态。
 */
LepusCStatus lepus_bigint_parse(LepusBigInt *output, const char *text, size_t size, unsigned base);
/**
 * @brief 生成独占文本，输出必须先零初始化或为已有转换结果。
 * @param output 文本目标。
 * @param value 数值。
 * @param base 零为默认十进制，其余为二至三十六。
 * @return 操作状态。
 */
LepusCStatus lepus_bigint_to_string(LepusBigString *output, const LepusBigInt *value, unsigned base);
/**
 * @brief 读取十进制位数。
 * @param value 数值。
 * @param digits 接收位数。
 * @param allocator 可选工作区分配器。
 * @return 操作状态。
 */
LepusCStatus lepus_bigint_decimal_digits(const LepusBigInt *value, size_t *digits, const LepusCAllocator *allocator);
/**
 * @brief 转为双精度近似。
 * @param value 数值。
 * @return 近似值。
 */
double lepus_bigint_to_double(const LepusBigInt *value);
/**
 * @brief 非负整数的按位操作。
 * @param output 目标。
 * @param left 左值。
 * @param right 右值。
 * @param operation 零与、一或、二异或。
 * @return 操作状态。
 */
LepusCStatus lepus_bigint_bitwise(LepusBigInt *output, const LepusBigInt *left, const LepusBigInt *right, int operation);
/**
 * @brief 读取幅值位。
 * @param value 数值。
 * @param bit 位下标。
 * @return 位值。
 */
int lepus_bigint_bit_at(const LepusBigInt *value, size_t bit);
/**
 * @brief 修改幅值位。
 * @param value 数值。
 * @param bit 位下标。
 * @param operation 零清除、一设置、二翻转。
 * @return 操作状态。
 */
LepusCStatus lepus_bigint_change_bit(LepusBigInt *value, size_t bit, int operation);
/**
 * @brief 整数快速幂。
 * @param output 结果。
 * @param value 底数。
 * @param exponent 指数。
 * @return 状态。
 */
LepusCStatus lepus_bigint_pow(LepusBigInt *output, const LepusBigInt *value, uint64_t exponent);
/**
 * @brief 最大公约数。
 * @param output 结果。
 * @param left 左值。
 * @param right 右值。
 * @return 状态。
 */
LepusCStatus lepus_bigint_gcd(LepusBigInt *output, const LepusBigInt *left, const LepusBigInt *right);
/**
 * @brief 最小公倍数。
 * @param output 结果。
 * @param left 左值。
 * @param right 右值。
 * @return 状态。
 */
LepusCStatus lepus_bigint_lcm(LepusBigInt *output, const LepusBigInt *left, const LepusBigInt *right);
/**
 * @brief 非负取模。
 * @param output 结果。
 * @param value 被除数。
 * @param modulus 模数。
 * @return 状态。
 */
LepusCStatus lepus_bigint_mod(LepusBigInt *output, const LepusBigInt *value, const LepusBigInt *modulus);
/**
 * @brief 大整数指数模幂。
 * @param output 结果。
 * @param value 底数。
 * @param exponent 指数。
 * @param modulus 模数。
 * @return 状态。
 */
LepusCStatus lepus_bigint_powmod(LepusBigInt *output, const LepusBigInt *value, const LepusBigInt *exponent, const LepusBigInt *modulus);
/**
 * @brief 扩展欧几里得，三个不同目标在全部成功后一起发布。
 * @param gcd 公约数。
 * @param x 左系数。
 * @param y 右系数。
 * @param left 左值。
 * @param right 右值。
 * @return 状态。
 */
LepusCStatus lepus_bigint_extended_gcd(LepusBigInt *gcd, LepusBigInt *x, LepusBigInt *y, const LepusBigInt *left, const LepusBigInt *right);
/**
 * @brief 非负模逆元。
 * @param output 目标。
 * @param value 数值。
 * @param modulus 模数。
 * @return 状态，不互素或零模数返回定义域错误。
 */
LepusCStatus lepus_bigint_inverse(LepusBigInt *output, const LepusBigInt *value, const LepusBigInt *modulus);
/**
 * @brief 固定底数序列的概率素性测试。
 * @param value 输入。
 * @param rounds 轮数。
 * @param probable 输出判定。
 * @param allocator 可选工作区分配器。
 * @return 状态。
 */
LepusCStatus lepus_bigint_is_probable_prime(const LepusBigInt *value, size_t rounds, int *probable, const LepusCAllocator *allocator);
/**
 * @brief 严格更大的概率素数。
 * @param output 目标。
 * @param value 输入。
 * @param rounds 检验轮数。
 * @return 状态。
 */
LepusCStatus lepus_bigint_next_probable_prime(LepusBigInt *output, const LepusBigInt *value, size_t rounds);
/**
 * @brief 整数平方根。
 * @param output 目标。
 * @param value 非负输入。
 * @return 状态。
 */
LepusCStatus lepus_bigint_sqrt(LepusBigInt *output, const LepusBigInt *value);
/**
 * @brief 整数 k 次根。
 * @param output 目标。
 * @param value 非负输入。
 * @param degree 正根次。
 * @return 状态。
 */
LepusCStatus lepus_bigint_root(LepusBigInt *output, const LepusBigInt *value, uint64_t degree);
/**
 * @brief 设置 NTT 阈值。
 * @param limbs 总分块阈值。
 */
void lepus_bigint_set_multiplication_threshold(size_t limbs);
/**
 * @brief 读取 NTT 阈值。
 * @return 总分块阈值。
 */
size_t lepus_bigint_multiplication_threshold(void);
/**
 * @brief 设置 Karatsuba 保护阈值。
 * @param limbs 短侧阈值。
 */
void lepus_bigint_set_karatsuba_threshold(size_t limbs);
/**
 * @brief 读取 Karatsuba 保护阈值。
 * @return 短侧阈值。
 */
size_t lepus_bigint_karatsuba_threshold(void);
/**
 * @brief 设置学校乘法候选的短侧分块上限，实际选择仍比较补齐成本；零表示关闭。
 * @param limbs 分块阈值。
 */
void lepus_bigint_set_schoolbook_threshold(size_t limbs);
/**
 * @brief 查询学校乘法候选的短侧分块上限。
 * @return 分块阈值。
 */
size_t lepus_bigint_schoolbook_threshold(void);
/**
 * @brief 设置专用学校平方的阈值，零表示关闭该基线。
 * @param limbs 分块阈值。
 */
void lepus_bigint_set_square_threshold(size_t limbs);
/**
 * @brief 查询学校平方阈值。
 * @return 分块阈值。
 */
size_t lepus_bigint_square_threshold(void);
/**
 * @brief 设置中等规模 Toom-Cook 3 乘法阈值，最小为八。
 * @param limbs 长侧分块阈值。
 */
void lepus_bigint_set_toom_threshold(size_t limbs);
/**
 * @brief 查询 Toom-Cook 3 乘法阈值。
 * @return 分块阈值。
 */
size_t lepus_bigint_toom_threshold(void);
/**
 * @brief 设置 Toom-Cook 3 平方阈值，最小为八。
 * @param limbs 分块阈值。
 */
void lepus_bigint_set_toom_square_threshold(size_t limbs);
/**
 * @brief 查询 Toom-Cook 3 平方阈值。
 * @return 分块阈值。
 */
size_t lepus_bigint_toom_square_threshold(void);
/**
 * @brief 设置除法递归阈值。
 * @param limbs 最小为三十二。
 */
void lepus_bigint_set_division_threshold(size_t limbs);
/**
 * @brief 读取除法递归阈值。
 * @return 分块阈值。
 */
size_t lepus_bigint_division_threshold(void);
#ifdef LEPUS_BIGINT_TESTING
/**
 * @brief 测试构建中强制顶层算法，递归层仍采用实际分派策略。
 * @param output 已初始化目标。
 * @param left 左值。
 * @param right 右值。
 * @param algorithm 零自动、一学校、二 Karatsuba、三 Toom、四 NTT。
 * @return 操作状态。
 */
LepusCStatus lepus_bigint_test_multiply(LepusBigInt *output, const LepusBigInt *left, const LepusBigInt *right, int algorithm);
/**
 * @brief 测试构建中查询实际自动分派算法。
 * @param left 左值。
 * @param right 右值。
 * @return 零输入返回零，其余算法编号与强制入口一致。
 */
int lepus_bigint_test_multiplication_route(const LepusBigInt *left, const LepusBigInt *right);
#endif
#ifdef __cplusplus
}
#endif
#endif
