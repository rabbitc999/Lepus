/**
 * @file abi_json.c
 * @brief JSON 与反射标量的纯 C 帧接口，反射开关与其他运行时翻译单元一致。
 */
#include "lepus_standard.h"
#include "../runtime_c/lepus_json.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static const char type_name[] = "std::json::JsonValue";
typedef enum JsonOperation
{
    New,
    Copy,
    Drop,
    Parse,
    Valid,
    Error,
    Close,
    Kind,
    Size,
    Int,
    Float,
    Bool,
    TextAvailable,
    String,
    Bytes,
    DateTime,
    Dump,
    Find,
    FindBytes,
    At,
    KeyAvailable,
    Key,
    KeyBytes,
    Compiled,
    Quote,
    FormatFloat,
    Finite,
    Number
} JsonOperation;
/**
 * @brief 通过 C 记录销毁协议释放 JSON。
 * @param value 拥有值、输入标量或待释放存储。
 */
static void LEPUS_ABI_CALL destroy_value(void *value) { lepus_json_free(value); }
/**
 * @brief 通过 C 记录复制协议创建独立 JSON。
 * @param value 拥有值、输入标量或待释放存储。
 * @return C 堆拥有结果，失败为空；调用者负责释放。
 */
static void *LEPUS_ABI_CALL clone_value(const void *value) { return lepus_json_clone(value); }
/**
 * @brief 完整构造后向语言运行时发布独占记录。
 * @param frame 当前 C ABI 调用帧。
 * @param value 拥有值、输入标量或待释放存储。
 * @return 当前查询或操作的结果，失败使用空或零状态。
 */
static int64_t make_value(const LepusAbiFrame *frame, LepusJson *value)
{
    int64_t handle;
    if (!value)
    {
        lepus_abi_fail(frame, "JSON 内核分配失败");
        return 0;
    }
    handle = lepus_abi_record_make_clonable(frame, type_name, value, destroy_value, clone_value);
    if (!handle) lepus_json_free(value);
    return handle;
}
/**
 * @brief 检查范围内当前位置是否为十进制数字。
 * @param text 输入字节。
 * @param length 明确字节数或接收结果字节数的指针。
 * @param at 起始字节位置。
 * @return 当前查询或操作的结果，失败使用空或零状态。
 */
static int digit(const unsigned char *text, size_t length, size_t at) { return at < length && text[at] >= '0' && text[at] <= '9'; }
/**
 * @brief 线性检查 JSON 数值文本并限制十进制指数。
 * @param text 输入字节。
 * @param length 明确字节数或接收结果字节数的指针。
 * @param decimal 是否允许小数与指数。
 * @return 当前查询或操作的结果，失败使用空或零状态。
 */
static int number_text(const unsigned char *text, size_t length, int decimal)
{
    size_t at = 0;
    uint64_t exponent = 0;
    if (at < length && text[at] == '-') at++;
    if (!digit(text, length, at)) return 0;
    if (text[at] == '0')
        at++;
    else
        while (digit(text, length, at)) at++;
    if (decimal && at < length && text[at] == '.')
    {
        at++;
        if (!digit(text, length, at)) return 0;
        while (digit(text, length, at)) at++;
    }
    if (decimal && at < length && (text[at] == 'e' || text[at] == 'E'))
    {
        at++;
        if (at < length && (text[at] == '+' || text[at] == '-')) at++;
        if (!digit(text, length, at)) return 0;
        while (digit(text, length, at))
        {
            exponent = exponent * 10 + (text[at++] - '0');
            if (exponent > 1000000) return 0;
        }
    }
    return at == length;
}
/**
 * @brief 先复制到语言字符串再释放 C 输出。
 * @param frame 当前 C ABI 调用帧。
 * @param text 输入字节。
 * @param length 明确字节数或接收结果字节数的指针。
 */
static void return_text(LepusAbiFrame *frame, char *text, size_t length)
{
    if (!text)
    {
        lepus_abi_fail(frame, "JSON 文本分配失败");
        return;
    }
    lepus_abi_return_integer(frame, lepus_abi_string(frame, text, length));
    free(text);
}
/**
 * @brief 核对帧参数并执行 C JSON 或反射标量操作。
 * @param frame 当前 C ABI 调用帧。
 * @param operation 固定操作编号。
 */
static void json_entry(LepusAbiFrame *frame, JsonOperation operation)
{
    int64_t handle = 0, ignored = 0, index = 0;
    LepusJson *value;
    const unsigned char *input = NULL;
    size_t length = 0;
    if (operation == Compiled)
    {
        lepus_abi_return_integer(frame, 1);
        return;
    }
    if (operation == FormatFloat || operation == Finite)
    {
        double number = 0;
        char *text;
        if (!lepus_abi_argument_real(frame, 0, &number)) return;
        if (operation == Finite)
        {
            lepus_abi_return_integer(frame, isfinite(number));
            return;
        }
        text = lepus_json_format_real(number, &length);
        return_text(frame, text, length);
        return;
    }
    if (operation == New)
    {
        lepus_abi_return_integer(frame, make_value(frame, lepus_json_new()));
        return;
    }
    if (!lepus_abi_argument_integer(frame, 0, &handle)) return;
    if (operation == Parse || operation == Quote || operation == Number)
    {
        if (!lepus_abi_bytes(frame, handle, &input, &length)) return;
        if (operation == Parse)
            lepus_abi_return_integer(frame, make_value(frame, lepus_json_parse((const char *)input, length)));
        else if (operation == Quote)
        {
            size_t size = 0;
            char *text = lepus_json_quote((const char *)input, length, &size);
            return_text(frame, text, size);
        }
        else
        {
            if (lepus_abi_argument_integer(frame, 1, &index)) lepus_abi_return_integer(frame, number_text(input, length, index != 0));
        }
        return;
    }
    if (operation == Copy)
    {
        lepus_abi_return_integer(frame, lepus_abi_record_clone(frame, handle, type_name, clone_value));
        return;
    }
    if (operation == Drop)
    {
        if (!lepus_abi_record_release(frame, handle)) lepus_abi_fail(frame, "[LEP1205] drop 的对象已经释放或句柄无效");
        lepus_abi_return_integer(frame, 0);
        return;
    }
    if ((operation == Valid || operation == Close) && !handle)
    {
        lepus_abi_return_integer(frame, 0);
        return;
    }
    if (operation >= Kind && !lepus_abi_argument_integer(frame, 1, &ignored)) return;
    if (operation >= Find && !lepus_abi_argument_integer(frame, 2, &index)) return;
    value = lepus_abi_record_borrow(frame, handle, type_name);
    if (!value)
    {
        lepus_abi_fail(frame, "结构化数据：值已经释放或类型无效");
        return;
    }
    if (operation == Valid)
    {
        lepus_abi_return_integer(frame, lepus_json_valid(value));
        return;
    }
    if (operation == Error)
    {
        const char *error = lepus_json_error(value);
        lepus_abi_return_integer(frame, lepus_abi_string(frame, error, strlen(error)));
        return;
    }
    if (operation == Close)
    {
        int valid = lepus_json_valid(value);
        lepus_json_close(value);
        lepus_abi_return_integer(frame, valid);
        return;
    }
    if (!lepus_json_valid(value))
    {
        lepus_abi_fail(frame, "结构化数据：值已经关闭或未初始化");
        return;
    }
    switch (operation)
    {
    case Kind: lepus_abi_return_integer(frame, lepus_json_kind(value)); break;
    case Size: lepus_abi_return_integer(frame, (int64_t)lepus_json_size(value)); break;
    case Int: lepus_abi_return_integer(frame, lepus_json_integer(value)); break;
    case Float: lepus_abi_return_real(frame, lepus_json_real(value)); break;
    case Bool: lepus_abi_return_integer(frame, lepus_json_boolean(value)); break;
    case TextAvailable:
    case String:
    case Bytes:
    {
        const char *text = lepus_json_text(value, &length);
        if (operation == TextAvailable)
            lepus_abi_return_integer(frame, text && !memchr(text, 0, length));
        else if (operation == String)
            lepus_abi_return_integer(frame, lepus_abi_string_bytes(frame, text ? text : "", length));
        else
            lepus_abi_return_integer(frame, lepus_abi_list_from_bytes(frame, (const unsigned char *)(text ? text : ""), length));
        break;
    }
    case DateTime: lepus_abi_return_integer(frame, lepus_abi_string(frame, "", 0)); break;
    case Dump:
    {
        char *text = lepus_json_dump(value, &length);
        return_text(frame, text, length);
        break;
    }
    case At: lepus_abi_return_integer(frame, make_value(frame, lepus_json_at(value, index < 0 ? SIZE_MAX : (size_t)index))); break;
    case Find:
    case FindBytes:
    {
        unsigned char *owned = NULL;
        LepusJson *child;
        if (operation == Find)
        {
            if (!lepus_abi_bytes(frame, index, &input, &length)) return;
        }
        else
        {
            if (!lepus_abi_list_bytes(frame, index, NULL, 0, &length)) return;
            owned = malloc(length ? length : 1);
            if (!owned)
            {
                lepus_abi_fail(frame, "JSON 键分配失败");
                return;
            }
            if (!lepus_abi_list_bytes(frame, index, owned, length, &length))
            {
                free(owned);
                return;
            }
            input = owned;
        }
        child = lepus_json_find(value, (const char *)(input ? input : (const unsigned char *)""), length);
        free(owned);
        lepus_abi_return_integer(frame, make_value(frame, child));
        break;
    }
    case Key:
    case KeyBytes:
    case KeyAvailable:
    {
        const char *key = lepus_json_key(value, index < 0 ? SIZE_MAX : (size_t)index, &length);
        if (operation == KeyAvailable)
            lepus_abi_return_integer(frame, key && !memchr(key, 0, length));
        else if (operation == Key)
            lepus_abi_return_integer(frame, lepus_abi_string_bytes(frame, key ? key : "", length));
        else
            lepus_abi_return_integer(frame, lepus_abi_list_from_bytes(frame, (const unsigned char *)(key ? key : ""), length));
        break;
    }
    default: break;
    }
}

/**
 * @brief 执行 std::json::__at 对应的固定 C 帧操作。
 * @param frame 当前 C ABI 调用帧。
 */
static void LEPUS_ABI_CALL json_at_entry(LepusAbiFrame *frame) { json_entry(frame, At); }

/**
 * @brief 执行 std::json::__bool 对应的固定 C 帧操作。
 * @param frame 当前 C ABI 调用帧。
 */
static void LEPUS_ABI_CALL json_bool_entry(LepusAbiFrame *frame) { json_entry(frame, Bool); }

/**
 * @brief 执行 std::json::__bytes 对应的固定 C 帧操作。
 * @param frame 当前 C ABI 调用帧。
 */
static void LEPUS_ABI_CALL json_bytes_entry(LepusAbiFrame *frame) { json_entry(frame, Bytes); }

/**
 * @brief 执行 std::json::__close 对应的固定 C 帧操作。
 * @param frame 当前 C ABI 调用帧。
 */
static void LEPUS_ABI_CALL json_close_entry(LepusAbiFrame *frame) { json_entry(frame, Close); }

/**
 * @brief 执行 std::json::__dateTime 对应的固定 C 帧操作。
 * @param frame 当前 C ABI 调用帧。
 */
static void LEPUS_ABI_CALL json_date_time_entry(LepusAbiFrame *frame) { json_entry(frame, DateTime); }

/**
 * @brief 执行 std::json::__dump 对应的固定 C 帧操作。
 * @param frame 当前 C ABI 调用帧。
 */
static void LEPUS_ABI_CALL json_dump_entry(LepusAbiFrame *frame) { json_entry(frame, Dump); }

/**
 * @brief 执行 std::json::__find 对应的固定 C 帧操作。
 * @param frame 当前 C ABI 调用帧。
 */
static void LEPUS_ABI_CALL json_find_entry(LepusAbiFrame *frame) { json_entry(frame, Find); }

/**
 * @brief 执行 std::json::__findBytes 对应的固定 C 帧操作。
 * @param frame 当前 C ABI 调用帧。
 */
static void LEPUS_ABI_CALL json_find_bytes_entry(LepusAbiFrame *frame) { json_entry(frame, FindBytes); }

/**
 * @brief 执行 std::json::__float 对应的固定 C 帧操作。
 * @param frame 当前 C ABI 调用帧。
 */
static void LEPUS_ABI_CALL json_float_entry(LepusAbiFrame *frame) { json_entry(frame, Float); }

/**
 * @brief 执行 std::json::__int 对应的固定 C 帧操作。
 * @param frame 当前 C ABI 调用帧。
 */
static void LEPUS_ABI_CALL json_int_entry(LepusAbiFrame *frame) { json_entry(frame, Int); }

/**
 * @brief 执行 std::json::__key 对应的固定 C 帧操作。
 * @param frame 当前 C ABI 调用帧。
 */
static void LEPUS_ABI_CALL json_key_entry(LepusAbiFrame *frame) { json_entry(frame, Key); }

/**
 * @brief 执行 std::json::__keyAvailable 对应的固定 C 帧操作。
 * @param frame 当前 C ABI 调用帧。
 */
static void LEPUS_ABI_CALL json_key_available_entry(LepusAbiFrame *frame) { json_entry(frame, KeyAvailable); }

/**
 * @brief 执行 std::json::__keyBytes 对应的固定 C 帧操作。
 * @param frame 当前 C ABI 调用帧。
 */
static void LEPUS_ABI_CALL json_key_bytes_entry(LepusAbiFrame *frame) { json_entry(frame, KeyBytes); }

/**
 * @brief 执行 std::json::__kind 对应的固定 C 帧操作。
 * @param frame 当前 C ABI 调用帧。
 */
static void LEPUS_ABI_CALL json_kind_entry(LepusAbiFrame *frame) { json_entry(frame, Kind); }

/**
 * @brief 执行 std::json::__parse 对应的固定 C 帧操作。
 * @param frame 当前 C ABI 调用帧。
 */
static void LEPUS_ABI_CALL json_parse_entry(LepusAbiFrame *frame) { json_entry(frame, Parse); }

/**
 * @brief 执行 std::json::__size 对应的固定 C 帧操作。
 * @param frame 当前 C ABI 调用帧。
 */
static void LEPUS_ABI_CALL json_size_entry(LepusAbiFrame *frame) { json_entry(frame, Size); }

/**
 * @brief 执行 std::json::__string 对应的固定 C 帧操作。
 * @param frame 当前 C ABI 调用帧。
 */
static void LEPUS_ABI_CALL json_string_entry(LepusAbiFrame *frame) { json_entry(frame, String); }

/**
 * @brief 执行 std::json::__takeError 对应的固定 C 帧操作。
 * @param frame 当前 C ABI 调用帧。
 */
static void LEPUS_ABI_CALL json_take_error_entry(LepusAbiFrame *frame) { json_entry(frame, Error); }

/**
 * @brief 执行 std::json::__textAvailable 对应的固定 C 帧操作。
 * @param frame 当前 C ABI 调用帧。
 */
static void LEPUS_ABI_CALL json_text_available_entry(LepusAbiFrame *frame) { json_entry(frame, TextAvailable); }

/**
 * @brief 执行 std::json::__valid 对应的固定 C 帧操作。
 * @param frame 当前 C ABI 调用帧。
 */
static void LEPUS_ABI_CALL json_valid_entry(LepusAbiFrame *frame) { json_entry(frame, Valid); }

/**
 * @brief 执行 std::json::__valueCopy 对应的固定 C 帧操作。
 * @param frame 当前 C ABI 调用帧。
 */
static void LEPUS_ABI_CALL json_value_copy_entry(LepusAbiFrame *frame) { json_entry(frame, Copy); }

/**
 * @brief 执行 std::json::__valueDrop 对应的固定 C 帧操作。
 * @param frame 当前 C ABI 调用帧。
 */
static void LEPUS_ABI_CALL json_value_drop_entry(LepusAbiFrame *frame) { json_entry(frame, Drop); }

/**
 * @brief 执行 std::json::__valueNew 对应的固定 C 帧操作。
 * @param frame 当前 C ABI 调用帧。
 */
static void LEPUS_ABI_CALL json_value_new_entry(LepusAbiFrame *frame) { json_entry(frame, New); }

/**
 * @brief 执行 std::reflect::__compiledReflection 对应的固定 C 帧操作。
 * @param frame 当前 C ABI 调用帧。
 */
static void LEPUS_ABI_CALL json_compiled_reflection_entry(LepusAbiFrame *frame) { json_entry(frame, Compiled); }

/**
 * @brief 执行 std::reflect::__serdeFinite 对应的固定 C 帧操作。
 * @param frame 当前 C ABI 调用帧。
 */
static void LEPUS_ABI_CALL json_serde_finite_entry(LepusAbiFrame *frame) { json_entry(frame, Finite); }

/**
 * @brief 执行 std::reflect::__serdeFloat 对应的固定 C 帧操作。
 * @param frame 当前 C ABI 调用帧。
 */
static void LEPUS_ABI_CALL json_serde_float_entry(LepusAbiFrame *frame) { json_entry(frame, FormatFloat); }

/**
 * @brief 执行 std::reflect::__serdeNumber 对应的固定 C 帧操作。
 * @param frame 当前 C ABI 调用帧。
 */
static void LEPUS_ABI_CALL json_serde_number_entry(LepusAbiFrame *frame) { json_entry(frame, Number); }

/**
 * @brief 执行 std::reflect::__serdeQuote 对应的固定 C 帧操作。
 * @param frame 当前 C ABI 调用帧。
 */
static void LEPUS_ABI_CALL json_serde_quote_entry(LepusAbiFrame *frame) { json_entry(frame, Quote); }

/**
 * @brief 返回 JSON 与反射标量服务的只读 C 入口表。
 * @param count 接收入口数量，可为空。
 * @return 当前查询或操作的结果，失败使用空或零状态。
 */
const LepusAbiRecord *LEPUS_ABI_CALL abi_json_records(size_t *count)
{
    static const LepusAbiRecord records[] = {
        { "std::json::__at", json_at_entry, 3 },
        { "std::json::__bool", json_bool_entry, 2 },
        { "std::json::__bytes", json_bytes_entry, 2 },
        { "std::json::__close", json_close_entry, 1 },
        { "std::json::__dateTime", json_date_time_entry, 2 },
        { "std::json::__dump", json_dump_entry, 2 },
        { "std::json::__find", json_find_entry, 3 },
        { "std::json::__findBytes", json_find_bytes_entry, 3 },
        { "std::json::__float", json_float_entry, 2 },
        { "std::json::__int", json_int_entry, 2 },
        { "std::json::__key", json_key_entry, 3 },
        { "std::json::__keyAvailable", json_key_available_entry, 3 },
        { "std::json::__keyBytes", json_key_bytes_entry, 3 },
        { "std::json::__kind", json_kind_entry, 2 },
        { "std::json::__parse", json_parse_entry, 1 },
        { "std::json::__size", json_size_entry, 2 },
        { "std::json::__string", json_string_entry, 2 },
        { "std::json::__takeError", json_take_error_entry, 1 },
        { "std::json::__textAvailable", json_text_available_entry, 2 },
        { "std::json::__valid", json_valid_entry, 1 },
        { "std::json::__valueCopy", json_value_copy_entry, 1 },
        { "std::json::__valueDrop", json_value_drop_entry, 1 },
        { "std::json::__valueNew", json_value_new_entry, 0 },
        { "std::reflect::__compiledReflection", json_compiled_reflection_entry, 0 },
        { "std::reflect::__serdeFinite", json_serde_finite_entry, 1 },
        { "std::reflect::__serdeFloat", json_serde_float_entry, 1 },
        { "std::reflect::__serdeNumber", json_serde_number_entry, 2 },
        { "std::reflect::__serdeQuote", json_serde_quote_entry, 1 }
    };
    if (count) *count = sizeof(records) / sizeof(records[0]);
    return records;
}

#ifndef GLUE_NO_STD_JSON
#define LEPUS_BUILD_BIND_MODULE json
#include "aot_bind_module.h"
#endif
