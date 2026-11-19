# std::toml — TOML 文档与反射编解码

<!-- LANGUAGE NAVIGATION -->
[English（中文原文）](std-toml.md) · **简体中文**
<!-- END LANGUAGE NAVIGATION -->

[返回库目录](README.md) · [全部符号](symbols.md)

## 引入

```lepus
import std::toml;
```

## 公共约定与错误边界

parse 返回 Result<TomlValue,MessageError>。与 JSON 共用拥有型 opcode 协议；普通复制深复制子树，close 只影响当前值。表对应 isTable/isObject；时间值使用 isDateTime/asDateTime。asXxx 类型不符返回 None；含零字节的文本使用字节列表接口。kind 编码与 JSON 节点一致。解析文档根节点的 dump() 保留原文注释、空白与换行；反复解析保存不丢失或重复注释。底层 C++ 等形值编辑保留布局；结构编辑保留注释内容，但可调整位置。子节点只输出值。

## 参考代码

完整程序：[example/samples/std_structured_data.lep](../../example/samples/std_structured_data.lep)。

```powershell
lepus run example/samples/std_structured_data.lep
```

<details>
<summary>展开完整参考代码</summary>

```lepus
/*
 * std_structured_data.lep —— `std::json` 与 `std::toml` 的读写样例。
 *
 * 本例将 find / keyAt / asString 的结果分别保存，方便逐层检查 Result/Option。
 * 当前语言也支持 root.find("x").unwrap() 等持资源结果的链式访问；编译器在完整
 * 表达式结束后清理中间值。节点拥有独立子树，close 不影响副本及已提取的子值。
 */
import std::json;
import std::toml;

void main() {
    Result<JsonValue, Error> parsed = std::json::parse(
    "{\"name\":\"中文\",\"items\":[1,2,3],\"active\":true,\"nothing\":null,"
    "\"ratio\":1.5,\"min\":-9223372036854775808}");
    JsonValue root = parsed.unwrap();
    assert(root.isObject() && root.len() == 6);
    Option<String> firstKey = root.keyAt(0);
    assert(firstKey.unwrap() == "name");
    Option<JsonValue> nameField = root.find("name");
    JsonValue nameValue = nameField.unwrap();
    Option<String> nameText = nameValue.asString();
    assert(nameText.unwrap() == "中文");

    Option<JsonValue> itemsField = root.find("items");
    JsonValue items = itemsField.unwrap();
    int64 sum = 0;
    for (JsonValue item in items) {
        Option<int64> itemNumber = item.asInt();
        sum = sum + itemNumber.unwrap();
    }
    assert(sum == 6 && items.at(-1).isNone() && items.at(3).isNone());
    Option<JsonValue> missingField = root.find("missing");
    assert(missingField.isNone());
    Option<JsonValue> activeField = root.find("active");
    JsonValue activeValue = activeField.unwrap();
    Option<bool> activeBool = activeValue.asBool();
    assert(activeBool.unwrap());
    Option<JsonValue> nothingField = root.find("nothing");
    JsonValue nothingValue = nothingField.unwrap();
    assert(nothingValue.isNull());
    Option<int64> nothingInt = nothingValue.asInt();
    assert(nothingInt.isNone());
    Option<JsonValue> ratioField = root.find("ratio");
    JsonValue ratioValue = ratioField.unwrap();
    Option<float64> ratioNumber = ratioValue.asFloat64();
    assert(ratioNumber.unwrap() == 1.5);
    Option<int64> ratioInt = ratioValue.asInt();
    assert(ratioInt.isNone());
    Option<JsonValue> minField = root.find("min");
    JsonValue minValue = minField.unwrap();
    Option<int64> minNumber = minValue.asInt();
    assert(format("{}", minNumber.unwrap()) == "-9223372036854775808");

    String rootText = root.dump();
    Result<JsonValue, Error> reparsed = std::json::parse(rootText);
    JsonValue reloaded = reparsed.unwrap();
    Option<JsonValue> reloadedName = reloaded.find("name");
    JsonValue reloadedNameValue = reloadedName.unwrap();
    Option<String> reloadedText = reloadedNameValue.asString();
    assert(reloadedText.unwrap() == "中文");
    assert(reloaded.close());
    assert(root.close() && items.isValid() && !root.close());

    Result<JsonValue, Error> parsedZero =
    std::json::parse("{\"a\\u0000b\":\"x\\u0000y\"}");
    JsonValue zero = parsedZero.unwrap();
    Option<String> zeroKey = zero.keyAt(0);
    assert(zeroKey.isNone());
    Option<List<uint8>> keyResult = zero.keyBytesAt(0);
    List<uint8> key = keyResult.unwrap();
    assert(key.len() == 3 && key[1] == (uint8)0);
    Option<JsonValue> zeroTextField = zero.findBytes(key);
    JsonValue zeroText = zeroTextField.unwrap();
    Option<String> zeroTextString = zeroText.asString();
    assert(zeroTextString.isNone());
    Option<List<uint8>> rawResult = zeroText.asBytes();
    List<uint8> raw = rawResult.unwrap();
    assert(raw.len() == 3 && raw[0] == (uint8)120 && raw[1] == (uint8)0 &&
    raw[2] == (uint8)121);
    assert(zero.close());
    Result<JsonValue, Error> badJson = std::json::parse("{\"x\":}");
    assert(badJson.isErr() && len(badJson.error().toString()) > 0);
    Result<JsonValue, Error> trailing = std::json::parse("[1,]");
    assert(trailing.isErr());

    Result<TomlValue, Error> parsedConfig = std::toml::parse(
    "title = \"Lepus\"\nreleased = 1979-05-27T07:32:00Z\n[server]\nport = 8080\n"
    "enabled = true\nvalues = [1, 2, 3]\n[[products]]\nname = \"one\"\n"
    "[[products]]\nname = \"two\"\n");
    TomlValue config = parsedConfig.unwrap();
    assert(config.isTable());
    Option<String> titleKey = config.keyAt(0);
    assert(titleKey.unwrap() == "title");
    Option<TomlValue> titleField = config.find("title");
    TomlValue titleValue = titleField.unwrap();
    Option<String> titleText = titleValue.asString();
    assert(titleText.unwrap() == "Lepus");
    Option<TomlValue> serverField = config.find("server");
    TomlValue server = serverField.unwrap();
    Option<TomlValue> portField = server.find("port");
    TomlValue portValue = portField.unwrap();
    Option<int64> portNumber = portValue.asInt();
    assert(portNumber.unwrap() == 8080);
    Option<TomlValue> enabledField = server.find("enabled");
    TomlValue enabledValue = enabledField.unwrap();
    Option<bool> enabledBool = enabledValue.asBool();
    assert(enabledBool.unwrap());
    Option<TomlValue> releasedField = config.find("released");
    TomlValue releasedValue = releasedField.unwrap();
    Option<String> releasedText = releasedValue.asDateTime();
    assert(releasedText.unwrap() == "1979-05-27T07:32:00Z");

    Option<TomlValue> productsField = config.find("products");
    TomlValue products = productsField.unwrap();
    assert(products.len() == 2);
    Option<TomlValue> secondProduct = products.at(1);
    TomlValue second = secondProduct.unwrap();
    Option<TomlValue> secondName = second.find("name");
    TomlValue secondNameValue = secondName.unwrap();
    Option<String> secondNameText = secondNameValue.asString();
    assert(secondNameText.unwrap() == "two");
    int64 members = 0;
    for (TomlValue item in server) { members = members + 1; }
    assert(members == 3);
    Option<String> missingMember = server.keyAt(3);
    assert(missingMember.isNone());

    String configText = config.dump();
    Result<TomlValue, Error> reparsedConfig = std::toml::parse(configText);
    TomlValue copy = reparsedConfig.unwrap();
    Option<TomlValue> copyServer = copy.find("server");
    TomlValue copyServerValue = copyServer.unwrap();
    Option<TomlValue> copyPort = copyServerValue.find("port");
    TomlValue copyPortValue = copyPort.unwrap();
    Option<int64> copyPortNumber = copyPortValue.asInt();
    assert(copyPortNumber.unwrap() == 8080);
    assert(copy.close() && config.close() && server.isValid());
    Result<TomlValue, Error> duplicate = std::toml::parse("x = 1\nx = 2\n");
    assert(duplicate.isErr() && len(duplicate.error().toString()) > 0);
    for (int64 i = 0; i < 50; i = i + 1) {

        Result<JsonValue, Error> parsedSmall = std::json::parse("[]");
        JsonValue small = parsedSmall.unwrap();
        assert(small.close());
        Result<TomlValue, Error> invalid = std::toml::parse("x = [");
        assert(invalid.isErr());

    }
    Result<JsonValue, Error> trailingOk = std::json::parse("true");
    assert(trailingOk.isOk()); // 注册表结束时清理未显式关闭的文档
    println("JSON and TOML parsing OK");
}
```

</details>

<details>
<summary>示例输出</summary>

```text
JSON and TOML parsing OK
```

</details>

聚合入口的具体操作参见对应子模块；本页类型/方法的调用形式列在下方。

## 宏定义

<a id="symbol-515"></a>

### `Toml`

```lepus
$macro Toml<T>(encode:ident, decode:ident) -> member {
```

为具体模型生成实例 TOML 编码和静态解码方法。

## 类型、枚举、常量与字段

公开字段按源码原样列出；句柄和游标字段用于包装器状态，通常通过方法操作。枚举的括号表示变体载荷。

| 所属类型 | 声明 | 说明 |
| --- | --- | --- |
| 模块 | <a id="symbol-516"></a>`struct TomlValue : Iterator<TomlValue>` | 独立拥有 TOML 子树的值，关闭不影响副本，根序列化保留原文注释。 |

## 函数索引

| 名称 | 声明 |
| --- | --- |
| [TomlValue::isValid](#api-0) | `bool isValid()` |
| [TomlValue::close](#api-1) | `bool close()` |
| [TomlValue::kind](#api-2) | `int64 kind()` |
| [TomlValue::isNull](#api-3) | `bool isNull()` |
| [TomlValue::isBool](#api-4) | `bool isBool()` |
| [TomlValue::isInt](#api-5) | `bool isInt()` |
| [TomlValue::isFloat](#api-6) | `bool isFloat()` |
| [TomlValue::isString](#api-7) | `bool isString()` |
| [TomlValue::isArray](#api-8) | `bool isArray()` |
| [TomlValue::isObject](#api-9) | `bool isObject()` |
| [TomlValue::isTable](#api-10) | `bool isTable()` |
| [TomlValue::isDateTime](#api-11) | `bool isDateTime()` |
| [TomlValue::len](#api-12) | `int64 len()` |
| [TomlValue::asInt](#api-13) | `Option<int64> asInt()` |
| [TomlValue::asFloat64](#api-14) | `Option<float64> asFloat64()` |
| [TomlValue::asBool](#api-15) | `Option<bool> asBool()` |
| [TomlValue::asString](#api-16) | `Option<String> asString()` |
| [TomlValue::asBytes](#api-17) | `Option<List<uint8>> asBytes()` |
| [TomlValue::asDateTime](#api-18) | `Option<String> asDateTime()` |
| [TomlValue::find](#api-19) | `Option<TomlValue> find(String key)` |
| [TomlValue::findBytes](#api-20) | `Option<TomlValue> findBytes(List<uint8> key)` |
| [TomlValue::at](#api-21) | `Option<TomlValue> at(int64 index)` |
| [TomlValue::iterator](#api-22) | `Option<TomlValue> iterator(int64 index)` |
| [TomlValue::keyAt](#api-23) | `Option<String> keyAt(int64 index)` |
| [TomlValue::keyBytesAt](#api-24) | `Option<List<uint8>> keyBytesAt(int64 index)` |
| [TomlValue::dump](#api-25) | `String dump()` |
| [parse](#api-26) | `Result<TomlValue, Error> parse(String text)` |
| [serialize](#api-27) | `Result<String,Error> serialize<T>(T const& value)` |
| [deserialize](#api-28) | `Result<T,Error> deserialize<T>(String text)` |

<a id="api-0"></a>

### TomlValue::isValid（1）

**声明**

```lepus
bool isValid();
```

**行为**

判断共享文档是否仍有效。

检查当前值是否有效。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`bool`。判断共享文档是否仍有效。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.isValid();
```

**源码**：[source/std/toml.lep:54](../../source/std/toml.lep#L54)。

**完整示例**：[example/samples/std_structured_data.lep](../../example/samples/std_structured_data.lep)。

<a id="api-1"></a>

### TomlValue::close（2）

**声明**

```lepus
bool close();
```

**行为**

关闭底层资源；文档节点会使同文档所有视图失效。

关闭当前值；副本与已提取的子值继续有效，重复关闭返回 false。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`bool`。关闭底层资源；文档节点会使同文档所有视图失效。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.close();
```

**源码**：[source/std/toml.lep:56](../../source/std/toml.lep#L56)。

**完整示例**：[example/samples/std_structured_data.lep](../../example/samples/std_structured_data.lep)。

<a id="api-2"></a>

### TomlValue::kind（3）

**声明**

```lepus
int64 kind();
```

**行为**

返回节点类型编号。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`int64`。返回节点类型编号。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.kind();
```

**源码**：[source/std/toml.lep:57](../../source/std/toml.lep#L57)。

**完整示例**：[example/samples/std_structured_data.lep](../../example/samples/std_structured_data.lep)。

<a id="api-3"></a>

### TomlValue::isNull（4）

**声明**

```lepus
bool isNull();
```

**行为**

判断是否为 null/NULL。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`bool`。判断是否为 null/NULL。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.isNull();
```

**源码**：[source/std/toml.lep:58](../../source/std/toml.lep#L58)。

**完整示例**：[example/samples/std_structured_data.lep](../../example/samples/std_structured_data.lep)。

<a id="api-4"></a>

### TomlValue::isBool（5）

**声明**

```lepus
bool isBool();
```

**行为**

判断是否为布尔节点。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`bool`。判断是否为布尔节点。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.isBool();
```

**源码**：[source/std/toml.lep:59](../../source/std/toml.lep#L59)。

**完整示例**：[example/samples/std_structured_data.lep](../../example/samples/std_structured_data.lep)。

<a id="api-5"></a>

### TomlValue::isInt（6）

**声明**

```lepus
bool isInt();
```

**行为**

判断是否为整数节点。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`bool`。判断是否为整数节点。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.isInt();
```

**源码**：[source/std/toml.lep:60](../../source/std/toml.lep#L60)。

**完整示例**：[example/samples/std_structured_data.lep](../../example/samples/std_structured_data.lep)。

<a id="api-6"></a>

### TomlValue::isFloat（7）

**声明**

```lepus
bool isFloat();
```

**行为**

判断是否为浮点节点。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`bool`。判断是否为浮点节点。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.isFloat();
```

**源码**：[source/std/toml.lep:61](../../source/std/toml.lep#L61)。

**完整示例**：[example/samples/std_structured_data.lep](../../example/samples/std_structured_data.lep)。

<a id="api-7"></a>

### TomlValue::isString（8）

**声明**

```lepus
bool isString();
```

**行为**

判断是否为字符串节点。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`bool`。判断是否为字符串节点。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.isString();
```

**源码**：[source/std/toml.lep:62](../../source/std/toml.lep#L62)。

**完整示例**：[example/samples/std_structured_data.lep](../../example/samples/std_structured_data.lep)。

<a id="api-8"></a>

### TomlValue::isArray（9）

**声明**

```lepus
bool isArray();
```

**行为**

判断是否为数组节点。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`bool`。判断是否为数组节点。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.isArray();
```

**源码**：[source/std/toml.lep:63](../../source/std/toml.lep#L63)。

**完整示例**：[example/samples/std_structured_data.lep](../../example/samples/std_structured_data.lep)。

<a id="api-9"></a>

### TomlValue::isObject（10）

**声明**

```lepus
bool isObject();
```

**行为**

判断是否为对象/表节点。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`bool`。判断是否为对象/表节点。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.isObject();
```

**源码**：[source/std/toml.lep:64](../../source/std/toml.lep#L64)。

**完整示例**：[example/samples/std_structured_data.lep](../../example/samples/std_structured_data.lep)。

<a id="api-10"></a>

### TomlValue::isTable（11）

**声明**

```lepus
bool isTable();
```

**行为**

判断是否为表/对象节点。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`bool`。判断是否为表/对象节点。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.isTable();
```

**源码**：[source/std/toml.lep:65](../../source/std/toml.lep#L65)。

**完整示例**：[example/samples/std_structured_data.lep](../../example/samples/std_structured_data.lep)。

<a id="api-11"></a>

### TomlValue::isDateTime（12）

**声明**

```lepus
bool isDateTime();
```

**行为**

判断是否为日期时间节点。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`bool`。判断是否为日期时间节点。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.isDateTime();
```

**源码**：[source/std/toml.lep:66](../../source/std/toml.lep#L66)。

**完整示例**：[example/samples/std_structured_data.lep](../../example/samples/std_structured_data.lep)。

<a id="api-12"></a>

### TomlValue::len（13）

**声明**

```lepus
int64 len();
```

**行为**

返回当前元素数量。

数组元素数或对象成员数，标量为零。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`int64`。返回当前元素数量。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.len();
```

**源码**：[source/std/toml.lep:68](../../source/std/toml.lep#L68)。

**完整示例**：[example/samples/std_structured_data.lep](../../example/samples/std_structured_data.lep)。

<a id="api-13"></a>

### TomlValue::asInt（14）

**声明**

```lepus
Option<int64> asInt();
```

**行为**

提取整数值；具体转换规则见所属类型。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`Option<int64>`。提取整数值；具体转换规则见所属类型。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.asInt();
```

**源码**：[source/std/toml.lep:69](../../source/std/toml.lep#L69)。

**完整示例**：[example/samples/std_structured_data.lep](../../example/samples/std_structured_data.lep)。

<a id="api-14"></a>

### TomlValue::asFloat64（15）

**声明**

```lepus
Option<float64> asFloat64();
```

**行为**

提取浮点值，也接受整数节点。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`Option<float64>`。提取浮点值，也接受整数节点。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.asFloat64();
```

**源码**：[source/std/toml.lep:73](../../source/std/toml.lep#L73)。

**完整示例**：[example/samples/std_structured_data.lep](../../example/samples/std_structured_data.lep)。

<a id="api-15"></a>

### TomlValue::asBool（16）

**声明**

```lepus
Option<bool> asBool();
```

**行为**

以 Option/Result 提取布尔值，类型不符走失败分支。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`Option<bool>`。以 Option/Result 提取布尔值，类型不符走失败分支。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.asBool();
```

**源码**：[source/std/toml.lep:77](../../source/std/toml.lep#L77)。

**完整示例**：[example/samples/std_structured_data.lep](../../example/samples/std_structured_data.lep)。

<a id="api-16"></a>

### TomlValue::asString（17）

**声明**

```lepus
Option<String> asString();
```

**行为**

提取不含零字节的字符串；否则 None。

非字符串或含零字节时返回 None；含零字节的文本使用 asBytes。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`Option<String>`。提取不含零字节的字符串；否则 None。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.asString();
```

**源码**：[source/std/toml.lep:82](../../source/std/toml.lep#L82)。

**完整示例**：[example/samples/std_structured_data.lep](../../example/samples/std_structured_data.lep)。

<a id="api-17"></a>

### TomlValue::asBytes（18）

**声明**

```lepus
Option<List<uint8>> asBytes();
```

**行为**

提取字符串的原始字节，包括零字节。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`Option<List<uint8>>`。提取字符串的原始字节，包括零字节。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.asBytes();
```

**源码**：[source/std/toml.lep:87](../../source/std/toml.lep#L87)。

**完整示例**：[example/samples/std_structured_data.lep](../../example/samples/std_structured_data.lep)。

<a id="api-18"></a>

### TomlValue::asDateTime（19）

**声明**

```lepus
Option<String> asDateTime();
```

**行为**

提取日期时间的文本表示。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`Option<String>`。提取日期时间的文本表示。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.asDateTime();
```

**源码**：[source/std/toml.lep:92](../../source/std/toml.lep#L92)。

**完整示例**：[example/samples/std_structured_data.lep](../../example/samples/std_structured_data.lep)。

<a id="api-19"></a>

### TomlValue::find（20）

**声明**

```lepus
Option<TomlValue> find(String key);
```

**行为**

查找首个匹配项，返回下标或可选载荷；具体未找到表示见返回类型及模块约定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `key` | `String key` | 查找键或下标 | 无 |

**返回值**

`Option<TomlValue>`。查找首个匹配项，返回下标或可选载荷；具体未找到表示见返回类型及模块约定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.find(key);
```

**源码**：[source/std/toml.lep:97](../../source/std/toml.lep#L97)。

**完整示例**：[example/samples/std_structured_data.lep](../../example/samples/std_structured_data.lep)。

<a id="api-20"></a>

### TomlValue::findBytes（21）

**声明**

```lepus
Option<TomlValue> findBytes(List<uint8> key);
```

**行为**

按原始字节键查找成员，失败 None。

允许包含零字节的 UTF-8 键名。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `key` | `List<uint8> key` | 查找键或下标 | 无 |

**返回值**

`Option<TomlValue>`。按原始字节键查找成员，失败 None。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.findBytes(key);
```

**源码**：[source/std/toml.lep:104](../../source/std/toml.lep#L104)。

**完整示例**：[example/samples/std_structured_data.lep](../../example/samples/std_structured_data.lep)。

<a id="api-21"></a>

### TomlValue::at（22）

**声明**

```lepus
Option<TomlValue> at(int64 index);
```

**行为**

读取从 0 开始的指定位置。

数组按索引读取；对象按插入顺序读取成员值。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `index` | `int64 index` | 从 0 开始的位置 | 无 |

**返回值**

`Option<TomlValue>`。读取从 0 开始的指定位置。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.at(index);
```

**源码**：[source/std/toml.lep:111](../../source/std/toml.lep#L111)。

**完整示例**：[example/samples/std_structured_data.lep](../../example/samples/std_structured_data.lep)。

<a id="api-22"></a>

### TomlValue::iterator（23）

**声明**

```lepus
Option<TomlValue> iterator(int64 index);
```

**行为**

读取指定迭代位置；无元素时返回 None。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `index` | `int64 index` | 从 0 开始的位置 | 无 |

**返回值**

`Option<TomlValue>`。读取指定迭代位置；无元素时返回 None。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.iterator(index);
```

**源码**：[source/std/toml.lep:117](../../source/std/toml.lep#L117)。

**完整示例**：[example/samples/std_structured_data.lep](../../example/samples/std_structured_data.lep)。

<a id="api-23"></a>

### TomlValue::keyAt（24）

**声明**

```lepus
Option<String> keyAt(int64 index);
```

**行为**

按遍历位置读取键。

对象成员的键名；越界或键中含零字节时返回 None。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `index` | `int64 index` | 从 0 开始的位置 | 无 |

**返回值**

`Option<String>`。按遍历位置读取键。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.keyAt(index);
```

**源码**：[source/std/toml.lep:119](../../source/std/toml.lep#L119)。

**完整示例**：[example/samples/std_structured_data.lep](../../example/samples/std_structured_data.lep)。

<a id="api-24"></a>

### TomlValue::keyBytesAt（25）

**声明**

```lepus
Option<List<uint8>> keyBytesAt(int64 index);
```

**行为**

按成员位置读取原始字节键。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `index` | `int64 index` | 从 0 开始的位置 | 无 |

**返回值**

`Option<List<uint8>>`。按成员位置读取原始字节键。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.keyBytesAt(index);
```

**源码**：[source/std/toml.lep:124](../../source/std/toml.lep#L124)。

**完整示例**：[example/samples/std_structured_data.lep](../../example/samples/std_structured_data.lep)。

<a id="api-25"></a>

### TomlValue::dump（26）

**声明**

```lepus
String dump();
```

**行为**

将节点序列化为文本。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`String`。将节点序列化为文本。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.dump();
```

**源码**：[source/std/toml.lep:129](../../source/std/toml.lep#L129)。

**完整示例**：[example/samples/std_structured_data.lep](../../example/samples/std_structured_data.lep)。

<a id="api-26"></a>

### parse（27）

**声明**

```lepus
Result<TomlValue, Error> parse(String text);
```

**行为**

解析TOML 文本；成功返回根节点，失败返回诊断字符串。

解析文本；失败返回包含行列信息的诊断。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `text` | `String text` | 输入文本，位置与长度通常按字节 | 无 |

**返回值**

`Result<TomlValue, Error>`。解析TOML 文本；成功返回根节点，失败返回诊断字符串。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
parse(text);
```

**源码**：[source/std/toml.lep:133](../../source/std/toml.lep#L133)。

**完整示例**：[example/samples/std_structured_data.lep](../../example/samples/std_structured_data.lep)。

<a id="api-27"></a>

### serialize（28）

**声明**

```lepus
Result<String,Error> serialize<T>(T const& value);
```

**行为**

按具体反射类型编码独立值，失败返回诊断。

按具体类型编码值，返回独立结果。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T const& value` | 输入值或写入的新值 | 无 |

**返回值**

`Result<String,Error>`。按具体反射类型编码独立值，失败返回诊断。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
std::toml::serialize<T>(value);
```

**源码**：[source/std/toml.lep:141](../../source/std/toml.lep#L141)。

**完整示例**：[example/samples/std_structured_data.lep](../../example/samples/std_structured_data.lep)。

<a id="api-28"></a>

### deserialize（29）

**声明**

```lepus
Result<T,Error> deserialize<T>(String text);
```

**行为**

按具体反射类型严格校验并恢复独立拥有的值。

按具体类型校验输入并恢复拥有型值。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `text` | `String text` | 输入文本，位置与长度通常按字节 | 无 |

**返回值**

`Result<T,Error>`。按具体反射类型严格校验并恢复独立拥有的值。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
std::toml::deserialize<T>(text);
```

**源码**：[source/std/toml.lep:145](../../source/std/toml.lep#L145)。

**完整示例**：[example/samples/std_structured_data.lep](../../example/samples/std_structured_data.lep)。
