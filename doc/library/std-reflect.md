# std::reflect — 反射

<!-- LANGUAGE NAVIGATION -->
[English（中文原文）](std-reflect.md) · **简体中文**
<!-- END LANGUAGE NAVIGATION -->

[返回库目录](README.md) · [全部符号](symbols.md)

## 引入

```lepus
import std::reflect;
```

## 公共约定与错误边界

要求工具链构建时同时启用 LEPUS_WITH_REFLECTION=1 与 RUNTIME_WITH_REFLECTION=1，并在编译/运行时启用 --reflection。类型信息在具体类型和布局确定后生成。ReflectValue 保存注解实参与默认值。

## 参考代码

完整程序：[doc/examples/library/reflect.lep](../../doc/examples/library/reflect.lep)。

```powershell
lepus run doc/examples/library/reflect.lep --reflection
```

需先用 `make reflection` 构建支持反射的工具链；当前二进制未编入支持时，上述命令会明确报缺失宏。

<details>
<summary>展开完整参考代码</summary>

```lepus
import std::reflect;

/** @brief 供启用反射的工具链演示类型元数据查询。 */
@Reflect
struct ReferenceRecord { int64 id; String name; };

void main() {
    uint64 id = std::reflect::getTypeId<ReferenceRecord>();
    Option<std::reflect::StructInfo> info = std::reflect::getStructInfo<ReferenceRecord>();
    assert(info.isValue());
    std::reflect::StructInfo record = info.unwrap();
    assert(record.typeId == id && record.fields.len() == 2);
    println("reflect API ok");

}
```

</details>

聚合入口的具体操作参见对应子模块；本页类型/方法的调用形式列在下方。

## 类型、枚举、常量与字段

公开字段按源码原样列出；句柄和游标字段用于包装器状态，通常通过方法操作。枚举的括号表示变体载荷。

| 所属类型 | 声明 | 说明 |
| --- | --- | --- |
| 模块 | <a id="symbol-391"></a>`enum ReflectError : Error` | 注解常量载荷的类型错误。 |
| ReflectError | `TypeMismatch` | 注解常量载荷的类型错误。 变体及载荷见声明。 |
| 模块 | <a id="symbol-392"></a>`enum ReflectValue` | 封闭值集合：精确保存位宽、符号及字符串所有权。 |
| ReflectValue | `Empty, Bool(bool value), Char(char value), I8(int8 value), I16(int16 value), I32(int32 value), I64(int64 value), U8(uint8 value), U16(uint16 value), U32(uint32 value), U64(uint64 value), F32(float32 value), F64(float64 value), Text(String value)` | 封闭值集合：精确保存位宽、符号及字符串所有权。 变体及载荷见声明。 |
| 模块 | <a id="symbol-393"></a>`struct ParameterInfo` | 反射参数的名称与类型身份。 |
| ParameterInfo | <a id="symbol-394"></a>`String name` | 声明或选项的名称。 |
| ParameterInfo | <a id="symbol-395"></a>`uint64 typeId` | 编译器登记的类型身份，不是对象地址。 |
| 模块 | <a id="symbol-396"></a>`struct AnnotationParameter` | 注解参数的名称、类型及可选默认值。 |
| AnnotationParameter | <a id="symbol-397"></a>`String name` | 声明或选项的名称。 |
| AnnotationParameter | <a id="symbol-398"></a>`uint64 typeId` | 编译器登记的类型身份，不是对象地址。 |
| AnnotationParameter | <a id="symbol-399"></a>`Option<std::reflect::ReflectValue> defaultValue` | 可选的注解默认实参；None 表示没有默认值。 |
| 模块 | <a id="symbol-400"></a>`struct AnnotationInfo` | 注解定义的名称、类型身份与参数声明。 |
| AnnotationInfo | <a id="symbol-401"></a>`String name` | 声明或选项的名称。 |
| AnnotationInfo | <a id="symbol-402"></a>`uint64 typeId` | 编译器登记的类型身份，不是对象地址。 |
| AnnotationInfo | <a id="symbol-403"></a>`List<std::reflect::AnnotationParameter> params` | 按声明顺序保存的参数信息。 |
| 模块 | <a id="symbol-404"></a>`struct Annotation` | 注解实例的定义身份与实际参数值。 |
| Annotation | <a id="symbol-405"></a>`uint64 annotationId` | 注解定义的身份。 |
| Annotation | <a id="symbol-406"></a>`List<std::reflect::ReflectValue> args` | 注解实例的实际参数值。 |
| 模块 | <a id="symbol-407"></a>`struct FuncInfo` | 函数反射信息，包含参数、返回类型和注解。 |
| FuncInfo | <a id="symbol-408"></a>`String name` | 声明或选项的名称。 |
| FuncInfo | <a id="symbol-409"></a>`uint64 typeId` | 编译器登记的类型身份，不是对象地址。 |
| FuncInfo | <a id="symbol-410"></a>`List<std::reflect::ParameterInfo> params` | 按声明顺序保存的参数信息。 |
| FuncInfo | <a id="symbol-411"></a>`uint64 returnTypeId` | 函数返回类型的身份。 |
| FuncInfo | <a id="symbol-412"></a>`List<std::reflect::Annotation> annotations` | 附加在该声明上的注解实例。 |
| 模块 | <a id="symbol-413"></a>`struct MethodInfo` | 方法反射信息，包含函数签名及 static、const、构造器等标志。 |
| MethodInfo | <a id="symbol-414"></a>`FuncInfo function` | 方法的完整函数签名与注解信息。 |
| MethodInfo | <a id="symbol-415"></a>`bool isStatic` | 方法是否为静态方法。 |
| MethodInfo | <a id="symbol-416"></a>`bool isConstructor` | 方法是否为构造器。 |
| MethodInfo | <a id="symbol-417"></a>`bool isConst` | 方法是否只读访问接收者。 |
| MethodInfo | <a id="symbol-418"></a>`bool hasDefaultBody` | 接口方法是否提供默认实现。 |
| 模块 | <a id="symbol-419"></a>`struct FieldInfo` | 实例字段反射信息，包含字节偏移、类型身份和拥有属性。 |
| FieldInfo | <a id="symbol-420"></a>`String name` | 声明或选项的名称。 |
| FieldInfo | <a id="symbol-421"></a>`uint32 offset` | 相对起始位置的字节偏移。 |
| FieldInfo | <a id="symbol-422"></a>`uint64 typeId` | 编译器登记的类型身份，不是对象地址。 |
| FieldInfo | <a id="symbol-423"></a>`bool isOwned` | 成员是否负责所存资源的所有权。 |
| FieldInfo | <a id="symbol-424"></a>`List<std::reflect::Annotation> annotations` | 附加在该声明上的注解实例。 |
| 模块 | <a id="symbol-425"></a>`struct StaticMemberInfo` | 静态成员反射信息，包含类型身份与拥有属性。 |
| StaticMemberInfo | <a id="symbol-426"></a>`String name` | 声明或选项的名称。 |
| StaticMemberInfo | <a id="symbol-427"></a>`uint64 typeId` | 编译器登记的类型身份，不是对象地址。 |
| StaticMemberInfo | <a id="symbol-428"></a>`bool isOwned` | 成员是否负责所存资源的所有权。 |
| StaticMemberInfo | <a id="symbol-429"></a>`List<std::reflect::Annotation> annotations` | 附加在该声明上的注解实例。 |
| 模块 | <a id="symbol-430"></a>`struct VariantInfo` | 枚举变体的名称、标签与载荷参数。 |
| VariantInfo | <a id="symbol-431"></a>`String name` | 声明或选项的名称。 |
| VariantInfo | <a id="symbol-432"></a>`uint32 tag` | 枚举变体的数值标签。 |
| VariantInfo | <a id="symbol-433"></a>`List<std::reflect::ParameterInfo> payloads` | 枚举变体的载荷参数声明。 |
| 模块 | <a id="symbol-434"></a>`struct StructInfo` | 结构体反射信息，包含字段、方法、静态成员、实现接口及注解。 |
| StructInfo | <a id="symbol-435"></a>`String name` | 声明或选项的名称。 |
| StructInfo | <a id="symbol-436"></a>`uint64 typeId` | 编译器登记的类型身份，不是对象地址。 |
| StructInfo | <a id="symbol-437"></a>`List<std::reflect::FieldInfo> fields` | 结构体的实例字段信息。 |
| StructInfo | <a id="symbol-438"></a>`List<std::reflect::MethodInfo> methods` | 类型或接口的方法信息。 |
| StructInfo | <a id="symbol-439"></a>`List<std::reflect::StaticMemberInfo> staticMembers` | 类型的静态成员信息。 |
| StructInfo | <a id="symbol-440"></a>`List<uint64> implementedInterfaces` | 该类型实现的接口身份。 |
| StructInfo | <a id="symbol-441"></a>`List<std::reflect::Annotation> annotations` | 附加在该声明上的注解实例。 |
| 模块 | <a id="symbol-442"></a>`struct EnumInfo` | 枚举反射信息，包含变体、方法、静态成员、实现接口及注解。 |
| EnumInfo | <a id="symbol-443"></a>`String name` | 声明或选项的名称。 |
| EnumInfo | <a id="symbol-444"></a>`uint64 typeId` | 编译器登记的类型身份，不是对象地址。 |
| EnumInfo | <a id="symbol-445"></a>`List<std::reflect::VariantInfo> variants` | 枚举的变体信息。 |
| EnumInfo | <a id="symbol-446"></a>`List<std::reflect::MethodInfo> methods` | 类型或接口的方法信息。 |
| EnumInfo | <a id="symbol-447"></a>`List<std::reflect::StaticMemberInfo> staticMembers` | 类型的静态成员信息。 |
| EnumInfo | <a id="symbol-448"></a>`List<uint64> implementedInterfaces` | 该类型实现的接口身份。 |
| EnumInfo | <a id="symbol-449"></a>`List<std::reflect::Annotation> annotations` | 附加在该声明上的注解实例。 |
| 模块 | <a id="symbol-450"></a>`struct InterfaceInfo` | 接口反射信息，包含方法、实现者类型及注解。 |
| InterfaceInfo | <a id="symbol-451"></a>`String name` | 声明或选项的名称。 |
| InterfaceInfo | <a id="symbol-452"></a>`uint64 typeId` | 编译器登记的类型身份，不是对象地址。 |
| InterfaceInfo | <a id="symbol-453"></a>`List<std::reflect::MethodInfo> methods` | 类型或接口的方法信息。 |
| InterfaceInfo | <a id="symbol-454"></a>`List<uint64> implementedBy` | 实现该接口的具体类型身份。 |
| InterfaceInfo | <a id="symbol-455"></a>`List<std::reflect::Annotation> annotations` | 附加在该声明上的注解实例。 |

## 函数索引

| 名称 | 声明 |
| --- | --- |
| [requireReflection](#api-0) | `void requireReflection()` |
| [ReflectError::toString](#api-1) | `String toString()` |
| [ReflectValue::typeId](#api-2) | `uint64 typeId()` |
| [ReflectValue::asBool](#api-3) | `Result<bool,Error> asBool()` |
| [ReflectValue::asChar](#api-4) | `Result<char,Error> asChar()` |
| [ReflectValue::asI8](#api-5) | `Result<int8,Error> asI8()` |
| [ReflectValue::asI16](#api-6) | `Result<int16,Error> asI16()` |
| [ReflectValue::asI32](#api-7) | `Result<int32,Error> asI32()` |
| [ReflectValue::asI64](#api-8) | `Result<int64,Error> asI64()` |
| [ReflectValue::asU8](#api-9) | `Result<uint8,Error> asU8()` |
| [ReflectValue::asU16](#api-10) | `Result<uint16,Error> asU16()` |
| [ReflectValue::asU32](#api-11) | `Result<uint32,Error> asU32()` |
| [ReflectValue::asU64](#api-12) | `Result<uint64,Error> asU64()` |
| [ReflectValue::asF32](#api-13) | `Result<float32,Error> asF32()` |
| [ReflectValue::asF64](#api-14) | `Result<float64,Error> asF64()` |
| [ReflectValue::asText](#api-15) | `Result<String,Error> asText()` |
| [getFuncInfo](#api-16) | `Option<std::reflect::FuncInfo> getFuncInfo(String name)` |
| [getAnnotationInfo](#api-17) | `Option<std::reflect::AnnotationInfo> getAnnotationInfo(String name)` |
| [getTypeId](#api-18) | `uint64 getTypeId<T>()` |
| [getInterfaceId](#api-19) | `uint64 getInterfaceId<I>()` |
| [getStructInfo](#api-20) | `Option<std::reflect::StructInfo> getStructInfo<T>()` |
| [getEnumInfo](#api-21) | `Option<std::reflect::EnumInfo> getEnumInfo<T>()` |
| [getInterfaceInfo](#api-22) | `Option<std::reflect::InterfaceInfo> getInterfaceInfo<I>()` |
| [implementsInterface](#api-23) | `bool implementsInterface<T>(uint64 interfaceId)` |
| [getAnnotations](#api-24) | `List<std::reflect::Annotation> getAnnotations<T>()` |
| [getFieldAnnotations](#api-25) | `List<std::reflect::Annotation> getFieldAnnotations<T>(String name)` |
| [getFuncAnnotations](#api-26) | `List<std::reflect::Annotation> getFuncAnnotations(String name)` |
| [getMethodAnnotations](#api-27) | `List<std::reflect::Annotation> getMethodAnnotations<T>(String name)` |
| [getStaticMemberAnnotations](#api-28) | `List<std::reflect::Annotation> getStaticMemberAnnotations<T>(String name)` |

<a id="api-0"></a>

### requireReflection（1）

**声明**

```lepus
void requireReflection();
```

**行为**

检查编译器是否编入反射；缺失时 panic。

反射裁剪构建调用依赖反射的高级接口时直接 panic。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

无返回值。

**错误与边界**

遵循本页“公共约定与错误边界”。
实现包含 panic 分支；调用前应处理空值或失败状态。

**调用形式**

```lepus
requireReflection();
```

**源码**：[source/std/reflect.lep:5](../../source/std/reflect.lep#L5)。

**完整示例**：[doc/examples/library/reflect.lep](../../doc/examples/library/reflect.lep)。

<a id="api-1"></a>

### ReflectError::toString（2）

**声明**

```lepus
String toString();
```

**行为**

由实现者将当前值转换为文本，Formattable 重载接收 style。

返回注解常量的类型不匹配诊断。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`String`。由实现者将当前值转换为文本，Formattable 重载接收 style。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.toString();
```

**源码**：[source/std/reflect.lep:14](../../source/std/reflect.lep#L14)。

**完整示例**：[doc/examples/library/reflect.lep](../../doc/examples/library/reflect.lep)。

<a id="api-2"></a>

### ReflectValue::typeId（3）

**声明**

```lepus
uint64 typeId();
```

**行为**

返回 ReflectValue 当前载荷的精确类型 ID。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`uint64`。返回 ReflectValue 当前载荷的精确类型 ID。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.typeId();
```

**源码**：[source/std/reflect.lep:33](../../source/std/reflect.lep#L33)。

**完整示例**：[doc/examples/library/reflect.lep](../../doc/examples/library/reflect.lep)。

<a id="api-3"></a>

### ReflectValue::asBool（4）

**声明**

```lepus
Result<bool,Error> asBool();
```

**行为**

精确提取 Bool 载荷；变体不匹配返回 Fail(TypeMismatch)，不进行隐式数值转换。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`Result<bool,Error>`。精确提取 Bool 载荷；变体不匹配返回 Fail(TypeMismatch)，不进行隐式数值转换。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.asBool();
```

**源码**：[source/std/reflect.lep:53](../../source/std/reflect.lep#L53)。

**完整示例**：[doc/examples/library/reflect.lep](../../doc/examples/library/reflect.lep)。

<a id="api-4"></a>

### ReflectValue::asChar（5）

**声明**

```lepus
Result<char,Error> asChar();
```

**行为**

精确提取 Char 载荷；变体不匹配返回 Fail(TypeMismatch)，不进行隐式数值转换。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`Result<char,Error>`。精确提取 Char 载荷；变体不匹配返回 Fail(TypeMismatch)，不进行隐式数值转换。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.asChar();
```

**源码**：[source/std/reflect.lep:58](../../source/std/reflect.lep#L58)。

**完整示例**：[doc/examples/library/reflect.lep](../../doc/examples/library/reflect.lep)。

<a id="api-5"></a>

### ReflectValue::asI8（6）

**声明**

```lepus
Result<int8,Error> asI8();
```

**行为**

精确提取 I8 载荷；变体不匹配返回 Fail(TypeMismatch)，不进行隐式数值转换。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`Result<int8,Error>`。精确提取 I8 载荷；变体不匹配返回 Fail(TypeMismatch)，不进行隐式数值转换。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.asI8();
```

**源码**：[source/std/reflect.lep:63](../../source/std/reflect.lep#L63)。

**完整示例**：[doc/examples/library/reflect.lep](../../doc/examples/library/reflect.lep)。

<a id="api-6"></a>

### ReflectValue::asI16（7）

**声明**

```lepus
Result<int16,Error> asI16();
```

**行为**

精确提取 I16 载荷；变体不匹配返回 Fail(TypeMismatch)，不进行隐式数值转换。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`Result<int16,Error>`。精确提取 I16 载荷；变体不匹配返回 Fail(TypeMismatch)，不进行隐式数值转换。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.asI16();
```

**源码**：[source/std/reflect.lep:68](../../source/std/reflect.lep#L68)。

**完整示例**：[doc/examples/library/reflect.lep](../../doc/examples/library/reflect.lep)。

<a id="api-7"></a>

### ReflectValue::asI32（8）

**声明**

```lepus
Result<int32,Error> asI32();
```

**行为**

精确提取 I32 载荷；变体不匹配返回 Fail(TypeMismatch)，不进行隐式数值转换。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`Result<int32,Error>`。精确提取 I32 载荷；变体不匹配返回 Fail(TypeMismatch)，不进行隐式数值转换。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.asI32();
```

**源码**：[source/std/reflect.lep:73](../../source/std/reflect.lep#L73)。

**完整示例**：[doc/examples/library/reflect.lep](../../doc/examples/library/reflect.lep)。

<a id="api-8"></a>

### ReflectValue::asI64（9）

**声明**

```lepus
Result<int64,Error> asI64();
```

**行为**

精确提取 I64 载荷；变体不匹配返回 Fail(TypeMismatch)，不进行隐式数值转换。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`Result<int64,Error>`。精确提取 I64 载荷；变体不匹配返回 Fail(TypeMismatch)，不进行隐式数值转换。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.asI64();
```

**源码**：[source/std/reflect.lep:78](../../source/std/reflect.lep#L78)。

**完整示例**：[doc/examples/library/reflect.lep](../../doc/examples/library/reflect.lep)。

<a id="api-9"></a>

### ReflectValue::asU8（10）

**声明**

```lepus
Result<uint8,Error> asU8();
```

**行为**

精确提取 U8 载荷；变体不匹配返回 Fail(TypeMismatch)，不进行隐式数值转换。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`Result<uint8,Error>`。精确提取 U8 载荷；变体不匹配返回 Fail(TypeMismatch)，不进行隐式数值转换。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.asU8();
```

**源码**：[source/std/reflect.lep:83](../../source/std/reflect.lep#L83)。

**完整示例**：[doc/examples/library/reflect.lep](../../doc/examples/library/reflect.lep)。

<a id="api-10"></a>

### ReflectValue::asU16（11）

**声明**

```lepus
Result<uint16,Error> asU16();
```

**行为**

精确提取 U16 载荷；变体不匹配返回 Fail(TypeMismatch)，不进行隐式数值转换。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`Result<uint16,Error>`。精确提取 U16 载荷；变体不匹配返回 Fail(TypeMismatch)，不进行隐式数值转换。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.asU16();
```

**源码**：[source/std/reflect.lep:88](../../source/std/reflect.lep#L88)。

**完整示例**：[doc/examples/library/reflect.lep](../../doc/examples/library/reflect.lep)。

<a id="api-11"></a>

### ReflectValue::asU32（12）

**声明**

```lepus
Result<uint32,Error> asU32();
```

**行为**

精确提取 U32 载荷；变体不匹配返回 Fail(TypeMismatch)，不进行隐式数值转换。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`Result<uint32,Error>`。精确提取 U32 载荷；变体不匹配返回 Fail(TypeMismatch)，不进行隐式数值转换。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.asU32();
```

**源码**：[source/std/reflect.lep:93](../../source/std/reflect.lep#L93)。

**完整示例**：[doc/examples/library/reflect.lep](../../doc/examples/library/reflect.lep)。

<a id="api-12"></a>

### ReflectValue::asU64（13）

**声明**

```lepus
Result<uint64,Error> asU64();
```

**行为**

精确提取 U64 载荷；变体不匹配返回 Fail(TypeMismatch)，不进行隐式数值转换。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`Result<uint64,Error>`。精确提取 U64 载荷；变体不匹配返回 Fail(TypeMismatch)，不进行隐式数值转换。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.asU64();
```

**源码**：[source/std/reflect.lep:98](../../source/std/reflect.lep#L98)。

**完整示例**：[doc/examples/library/reflect.lep](../../doc/examples/library/reflect.lep)。

<a id="api-13"></a>

### ReflectValue::asF32（14）

**声明**

```lepus
Result<float32,Error> asF32();
```

**行为**

精确提取 F32 载荷；变体不匹配返回 Fail(TypeMismatch)，不进行隐式数值转换。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`Result<float32,Error>`。精确提取 F32 载荷；变体不匹配返回 Fail(TypeMismatch)，不进行隐式数值转换。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.asF32();
```

**源码**：[source/std/reflect.lep:103](../../source/std/reflect.lep#L103)。

**完整示例**：[doc/examples/library/reflect.lep](../../doc/examples/library/reflect.lep)。

<a id="api-14"></a>

### ReflectValue::asF64（15）

**声明**

```lepus
Result<float64,Error> asF64();
```

**行为**

精确提取 F64 载荷；变体不匹配返回 Fail(TypeMismatch)，不进行隐式数值转换。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`Result<float64,Error>`。精确提取 F64 载荷；变体不匹配返回 Fail(TypeMismatch)，不进行隐式数值转换。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.asF64();
```

**源码**：[source/std/reflect.lep:108](../../source/std/reflect.lep#L108)。

**完整示例**：[doc/examples/library/reflect.lep](../../doc/examples/library/reflect.lep)。

<a id="api-15"></a>

### ReflectValue::asText（16）

**声明**

```lepus
Result<String,Error> asText();
```

**行为**

精确提取 Text 载荷；变体不匹配返回 Fail(TypeMismatch)，不进行隐式数值转换。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`Result<String,Error>`。精确提取 Text 载荷；变体不匹配返回 Fail(TypeMismatch)，不进行隐式数值转换。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.asText();
```

**源码**：[source/std/reflect.lep:113](../../source/std/reflect.lep#L113)。

**完整示例**：[doc/examples/library/reflect.lep](../../doc/examples/library/reflect.lep)。

<a id="api-16"></a>

### getFuncInfo（17）

**声明**

```lepus
Option<std::reflect::FuncInfo> getFuncInfo(String name);
```

**行为**

按名称查找函数元数据，未找到时 None。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `name` | `String name` | 名称 | 无 |

**返回值**

`Option<std::reflect::FuncInfo>`。按名称查找函数元数据，未找到时 None。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
getFuncInfo(name);
```

**源码**：[source/std/reflect.lep:138](../../source/std/reflect.lep#L138)。

**完整示例**：[doc/examples/library/reflect.lep](../../doc/examples/library/reflect.lep)。

<a id="api-17"></a>

### getAnnotationInfo（18）

**声明**

```lepus
Option<std::reflect::AnnotationInfo> getAnnotationInfo(String name);
```

**行为**

按名称查找注解定义元数据。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `name` | `String name` | 名称 | 无 |

**返回值**

`Option<std::reflect::AnnotationInfo>`。按名称查找注解定义元数据。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
getAnnotationInfo(name);
```

**源码**：[source/std/reflect.lep:139](../../source/std/reflect.lep#L139)。

**完整示例**：[doc/examples/library/reflect.lep](../../doc/examples/library/reflect.lep)。

<a id="api-18"></a>

### getTypeId（19）

**声明**

```lepus
uint64 getTypeId<T>();
```

**行为**

取得具体 T 的类型 ID。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`uint64`。取得具体 T 的类型 ID。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
getTypeId<T>();
```

**源码**：[source/std/reflect.lep:141](../../source/std/reflect.lep#L141)。

**完整示例**：[doc/examples/library/reflect.lep](../../doc/examples/library/reflect.lep)。

<a id="api-19"></a>

### getInterfaceId（20）

**声明**

```lepus
uint64 getInterfaceId<I>();
```

**行为**

取得接口 I 的类型 ID。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`uint64`。取得接口 I 的类型 ID。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
getInterfaceId<I>();
```

**源码**：[source/std/reflect.lep:142](../../source/std/reflect.lep#L142)。

**完整示例**：[doc/examples/library/reflect.lep](../../doc/examples/library/reflect.lep)。

<a id="api-20"></a>

### getStructInfo（21）

**声明**

```lepus
Option<std::reflect::StructInfo> getStructInfo<T>();
```

**行为**

读取具体结构体的布局和成员元数据。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`Option<std::reflect::StructInfo>`。读取具体结构体的布局和成员元数据。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
getStructInfo<T>();
```

**源码**：[source/std/reflect.lep:143](../../source/std/reflect.lep#L143)。

**完整示例**：[doc/examples/library/reflect.lep](../../doc/examples/library/reflect.lep)。

<a id="api-21"></a>

### getEnumInfo（22）

**声明**

```lepus
Option<std::reflect::EnumInfo> getEnumInfo<T>();
```

**行为**

读取枚举变体及成员元数据。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`Option<std::reflect::EnumInfo>`。读取枚举变体及成员元数据。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
getEnumInfo<T>();
```

**源码**：[source/std/reflect.lep:144](../../source/std/reflect.lep#L144)。

**完整示例**：[doc/examples/library/reflect.lep](../../doc/examples/library/reflect.lep)。

<a id="api-22"></a>

### getInterfaceInfo（23）

**声明**

```lepus
Option<std::reflect::InterfaceInfo> getInterfaceInfo<I>();
```

**行为**

读取接口方法及实现关系元数据。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`Option<std::reflect::InterfaceInfo>`。读取接口方法及实现关系元数据。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
getInterfaceInfo<I>();
```

**源码**：[source/std/reflect.lep:145](../../source/std/reflect.lep#L145)。

**完整示例**：[doc/examples/library/reflect.lep](../../doc/examples/library/reflect.lep)。

<a id="api-23"></a>

### implementsInterface（24）

**声明**

```lepus
bool implementsInterface<T>(uint64 interfaceId);
```

**行为**

判断 T 是否实现给定接口 ID。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `interfaceId` | `uint64 interfaceId` | 接口类型 ID | 无 |

**返回值**

`bool`。判断 T 是否实现给定接口 ID。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
implementsInterface<T>(interfaceId);
```

**源码**：[source/std/reflect.lep:146](../../source/std/reflect.lep#L146)。

**完整示例**：[doc/examples/library/reflect.lep](../../doc/examples/library/reflect.lep)。

<a id="api-24"></a>

### getAnnotations（25）

**声明**

```lepus
List<std::reflect::Annotation> getAnnotations<T>();
```

**行为**

取得类型上的注解列表。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`List<std::reflect::Annotation>`。取得类型上的注解列表。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
getAnnotations<T>();
```

**源码**：[source/std/reflect.lep:153](../../source/std/reflect.lep#L153)。

**完整示例**：[doc/examples/library/reflect.lep](../../doc/examples/library/reflect.lep)。

<a id="api-25"></a>

### getFieldAnnotations（26）

**声明**

```lepus
List<std::reflect::Annotation> getFieldAnnotations<T>(String name);
```

**行为**

按字段名取得字段注解。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `name` | `String name` | 名称 | 无 |

**返回值**

`List<std::reflect::Annotation>`。按字段名取得字段注解。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
getFieldAnnotations<T>(name);
```

**源码**：[source/std/reflect.lep:159](../../source/std/reflect.lep#L159)。

**完整示例**：[doc/examples/library/reflect.lep](../../doc/examples/library/reflect.lep)。

<a id="api-26"></a>

### getFuncAnnotations（27）

**声明**

```lepus
List<std::reflect::Annotation> getFuncAnnotations(String name);
```

**行为**

按函数名取得函数注解。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `name` | `String name` | 名称 | 无 |

**返回值**

`List<std::reflect::Annotation>`。按函数名取得函数注解。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
getFuncAnnotations(name);
```

**源码**：[source/std/reflect.lep:164](../../source/std/reflect.lep#L164)。

**完整示例**：[doc/examples/library/reflect.lep](../../doc/examples/library/reflect.lep)。

<a id="api-27"></a>

### getMethodAnnotations（28）

**声明**

```lepus
List<std::reflect::Annotation> getMethodAnnotations<T>(String name);
```

**行为**

按方法名取得方法注解。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `name` | `String name` | 名称 | 无 |

**返回值**

`List<std::reflect::Annotation>`。按方法名取得方法注解。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
getMethodAnnotations<T>(name);
```

**源码**：[source/std/reflect.lep:174](../../source/std/reflect.lep#L174)。

**完整示例**：[doc/examples/library/reflect.lep](../../doc/examples/library/reflect.lep)。

<a id="api-28"></a>

### getStaticMemberAnnotations（29）

**声明**

```lepus
List<std::reflect::Annotation> getStaticMemberAnnotations<T>(String name);
```

**行为**

按静态成员名取得注解。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `name` | `String name` | 名称 | 无 |

**返回值**

`List<std::reflect::Annotation>`。按静态成员名取得注解。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
getStaticMemberAnnotations<T>(name);
```

**源码**：[source/std/reflect.lep:178](../../source/std/reflect.lep#L178)。

**完整示例**：[doc/examples/library/reflect.lep](../../doc/examples/library/reflect.lep)。
