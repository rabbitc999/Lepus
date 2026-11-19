# std::core — 核心库

<!-- LANGUAGE NAVIGATION -->
[English（中文原文）](std-core.md) · **简体中文**
<!-- END LANGUAGE NAVIGATION -->

[返回库目录](README.md) · [全部符号](symbols.md)

## 引入

```lepus
// std::core 自动加载
```

## 公共约定与错误边界

自动加载，无需 import。错误输出包装和浮点去尾零由 Lepus 实现；LLVM 构建的标准流直连运行时与 libc，显式执行步数限制保留宿主回退。标准库失败返回统一为 Result<T, Error>，具体错误实现 Error.toString()；文本诊断使用 MessageError，保留原始消息。String 提供字节查找、切片、替换、修剪、填充、连接、分割、ASCII 大小写与分类，以及严格 UTF-8 码点操作；普通方法返回独立值，字节索引与码点索引明确区分，详情见字符串方法文档。Option/Result 通过 match 或 isValue/isOk 分支处理；unwrap/expect 在失败分支触发 panic。浮点转整数向零截断，范围检查见各重载；字节编码默认小端。拥有值按值复制，引用的生命周期不得超过来源。 Box<T> 独占堆上的一个值，由 Box<T>::newBox(value) 装箱。get 复制值，take 消耗盒并移出值；两者在空盒上触发断言。close 和作用域退出清理剩余值。List.pop 空列表报错，列表/集合/映射的按位置访问需有效下标。Set/Map 的 has/contains 返回 int64 的 0/1；List 的同名接口返回 bool。Any.getReference/getOptional 的 const/mut 由接收上下文决定，返回引用受盒子寿命限制。内存内建接受有效存储引用，字节计数不是元素数量。$assert(condition) 和 $assert(condition,message) 为宏调用，自动添加调用点行号，宏体自带分号。

## 参考代码

完整程序：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

```powershell
lepus run example/samples/std_collections_algorithms.lep
```

<details>
<summary>展开完整参考代码</summary>

```lepus
import std::collections;

import std::memory;

import std::algorithms;

bool positive(int64 value) { return value > 0; }
int64 addValues(int64 left, int64 right) { return left + right; }

void main() {
    List<int64> values;
    values.push(4);
    values.push(1);
    values.push(3);
    List<int64> sorted = sortValue(values);
    println("{} {} {} {} {}", sorted[0], sorted[1], sorted[2],
    binarySearchValue(sorted, 3),
    foldValue(sorted, 0, addValues));
    println("{} {} {}", values[0], values[1], values[2]);

    List<float64> stableValues;
    stableValues.push(-0.0);
    stableValues.push(2.0);
    stableValues.push(0.0);
    stableValues.push(1.0);
    List<float64> stableSorted = sortValue(stableValues);
    println("{1:1F} {2:1F} {3:1F} {4:1F}", stableSorted[0], stableSorted[1],
    stableSorted[2], stableSorted[3]);
    println("{} {} {} {} {}", findValue(sorted, 4),
    minValue(9, 2), maxValue(9, 2),
    clampValue(12, 0, 10),
    allValue(sorted, positive));
    println("{} {} {} {}", minValue(9, 2, 7, -1),
    maxValue(9, 2, 12, 7),
    minValue(5), maxValue(6));

    Deque<int64> deque = Deque<int64>::create();
    deque.pushBack(2);
    deque.pushFront(1);
    println("{} {} {}", deque.len(), deque.popFront().unwrap(),
    deque.popBack().unwrap());

    Queue<int64> queue = Queue<int64>::create();
    queue.push(7);
    queue.push(8);
    println("{} {}", queue.peek().unwrap(), queue.pop().unwrap());

    RingBuffer<int64> ring = RingBuffer<int64>::withCapacity(2);
    println("{} {} {}", ring.push(5), ring.push(6), ring.push(7));
    println("{} {}", ring.pop().unwrap(), ring.available());

    FixedVector<int64> fixed = FixedVector<int64>::withCapacity(1);
    println("{} {} {}", fixed.push(9), fixed.push(10), fixed[0]);

    int64 raw[] = new[2];
    raw[0] = 11;
    raw[1] = 12;
    Span<int64> span = Span<int64>::from(&raw[0], len(raw));
    println("{} {} {}", span.len(), span[0], span[1]);

    Channel<int64> channel = Channel<int64>::create();
    channel.send(21);
    println("{} {}", channel.len(), channel.receive().unwrap());

    Pair<int64, String> pair = Pair<int64, String>::create(7, "seven");
    println("{} {}", pair.first, pair.second);
    raw.drop();
}
```

</details>

<details>
<summary>示例输出</summary>

```text
1 3 4 1 8
4 1 3
-0.0 0.0 1.0 2.0
2 2 9 10 1
-1 12 5 6
2 1 2
7 7
1 1 0
5 1
1 0 9
2 11 12
1 21
7 seven
```

</details>

聚合入口的具体操作参见对应子模块；本页类型/方法的调用形式列在下方。

## 宏定义

<a id="symbol-63"></a>

### `assert`

```lepus
$macro assert(condition) -> {
```

系统断言宏：`$assert(condition)` / `$assert(condition, message)` 把**调用点**的行号 喂给上面带 `int64 line` 的重载。`$line` 在宏展开的位置取值，所以拿到的是调用处那一 行；限定名 `std::core::assert` 不会被这个同名宏吃掉（`.name` / `::name` 不当宏调用）。 宏体保存完整语句；调用处也写分号，与其他语句宏保持一致。 定义放在文件末尾：它上面那几行是 `assert` 函数自己的**定义**，不能被动到。

<a id="symbol-64"></a>

### `assert`

```lepus
$macro assert(condition, message) -> {
```

检查条件；失败时报告消息和宏调用处的源码行。

## 类型、枚举、常量与字段

公开字段按源码原样列出；句柄和游标字段用于包装器状态，通常通过方法操作。枚举的括号表示变体载荷。

| 所属类型 | 声明 | 说明 |
| --- | --- | --- |
| 模块 | <a id="symbol-65"></a>`opcode String` | 拥有 UTF-8 文本的 opcode 值，复制产生独立可写缓冲。 |
| 模块 | <a id="symbol-66"></a>`interface Debug` | 开发时的结构化显示协议；输出格式不保证跨版本稳定。 |
| 模块 | <a id="symbol-67"></a>`interface Error` | 标准库错误的统一接口；诊断文本保留具体失败原因。 |
| 模块 | <a id="symbol-68"></a>`struct MessageError : Error` | 只有文本诊断时使用的标准错误对象，消息按值独立拥有。 |
| MessageError | <a id="symbol-69"></a>`String message` | 供用户阅读的错误原因。 |
| 模块 | <a id="symbol-70"></a>`#NonCopyable opcode File` | 不可复制的文件 opcode，作用域退出释放文件资源。 |
| 模块 | <a id="symbol-71"></a>`#NonCopyable opcode Process` | 不可复制的进程 opcode，操作由宿主胶水提供。 |
| 模块 | <a id="symbol-72"></a>`enum TryBranch<T, R>` | 传播分支：Continue 产生成功值，Break 携带失败载荷退出当前函数。 |
| TryBranch | `Continue(T value), Break(R residual)` | 传播分支：Continue 产生成功值，Break 携带失败载荷退出当前函数。 变体及载荷见声明。 |
| 模块 | <a id="symbol-73"></a>`struct ErrorResidual<E>` | Result 的错误传播载荷，保留错误类型身份。 |
| ErrorResidual | <a id="symbol-74"></a>`E error` | 最近操作的错误说明。 |
| 模块 | <a id="symbol-75"></a>`enum NoneResidual` | Option 无值分支的传播载荷。 |
| NoneResidual | `None` | Option 无值分支的传播载荷。 变体及载荷见声明。 |
| 模块 | <a id="symbol-76"></a>`interface Try<T, R>` | 只读拆分传播值的显式编译期协议。 |
| 模块 | <a id="symbol-77"></a>`interface TryMove<T, R>` | 移动拥有值后拆分传播值的显式编译期协议。 |
| 模块 | <a id="symbol-78"></a>`interface FromResidual<F, R>` | 以静态方法从传播载荷构造函数返回值的编译期协议。 |
| 模块 | <a id="symbol-79"></a>`enum Option<T> : Try<T, NoneResidual>, TryMove<T, NoneResidual>, FromResidual<Option<T>, NoneResidual>` | 可选值：Value(T) 表示存在，None 表示缺失。 |
| Option | `Value(T value), None` | 可选值：Value(T) 表示存在，None 表示缺失。 变体及载荷见声明。 |
| 模块 | <a id="symbol-80"></a>`enum Result<T, E> : Try<T, ErrorResidual<E>>, TryMove<T, ErrorResidual<E>>, FromResidual<Result<T, E>, ErrorResidual<E>>` | 操作结果：Ok(T) 表示成功，Fail(E) 保存错误。 |
| Result | `Ok(T value), Fail(E error)` | 操作结果：Ok(T) 表示成功，Fail(E) 保存错误。 变体及载荷见声明。 |
| 模块 | <a id="symbol-alias-StdResult"></a>`type alias StdResult<T> = Result<T, Error>` | 将失败载荷固定为 Error 的透明别名，展开为 Result<T, Error>。 |
| 模块 | <a id="symbol-81"></a>`interface NextValue<T>` | 自定义区间的有状态前进协议。
@note nextValue 从接收者的游标状态产生下一项并推进状态；到达终点后返回 None。 |
| 模块 | <a id="symbol-82"></a>`struct Pair<K, V>` | 包含 first 和 second 的两个值，分别使用 K 与 V 类型。 |
| Pair | <a id="symbol-83"></a>`K first` | 第一项，类型为 K。 |
| Pair | <a id="symbol-84"></a>`V second` | 第二项，类型为 V。 |
| 模块 | <a id="symbol-85"></a>`struct RawPtr<T>` | 有元素类型及分配身份的原始指针；复制指针不复制元素，也不延长存储寿命。 |
| RawPtr | <a id="symbol-86"></a>`uint64 storage` | 对应原始存储的分配身份，不延长存储寿命。 |
| RawPtr | <a id="symbol-87"></a>`int64 index` | 当前元素序号，从 0 开始。 |
| 模块 | <a id="symbol-88"></a>`struct BarePtr<T>: Add<BarePtr<T>, int64>, RightAdd<BarePtr<T>, int64>, Subtract<BarePtr<T>, int64>, Subtract<int64, BarePtr<T>>` | 保存逻辑地址及可追踪的分配身份，不记录元素类型或初始化状态。 |
| BarePtr | <a id="symbol-89"></a>`int64 address` | 运行时逻辑字节地址；不是可传给宿主 C 函数的机器地址。 |
| BarePtr | <a id="symbol-90"></a>`uint64 storage` | 来源原始存储的身份，零表示没有可追踪的来源。 |
| 模块 | <a id="symbol-91"></a>`struct RawStorage<T>: Closeable` | 独占原始容量；备用槽位不默认构造 T，关闭时仅清理已初始化元素。 |
| RawStorage | <a id="symbol-92"></a>`uint64 handle` | 宿主资源句柄；应通过所属类型的方法操作。 |
| 模块 | <a id="symbol-93"></a>`struct Box<T>: Closeable` | 独占堆上的一个值。默认构造为空，移动时转移所有权，离开作用域时自动释放。 |
| 模块 | <a id="symbol-94"></a>`interface Drop` | 显式释放拥有资源的析构协议。 |
| 模块 | <a id="symbol-95"></a>`interface Closeable` | 关闭外部资源的协议。 |
| 模块 | <a id="symbol-96"></a>`struct List<T>` | 可变长度的顺序值容器，支持按下标访问、追加、删除与复制。 |
| 模块 | <a id="symbol-97"></a>`struct Set<T>` | 保存唯一元素的值集合，支持存在检查和集合运算。 |
| 模块 | <a id="symbol-98"></a>`struct Map<K,V>` | 从唯一键到值的映射，支持键查询和按遍历位置读取。 |
| 模块 | <a id="symbol-99"></a>`struct Any` | 拥有一个装箱值的类型擦除容器；通过类型检查安全读取。 |
| 模块 | <a id="symbol-100"></a>`interface Add<R,T>` | 左操作数的加法重载接口。 |
| 模块 | <a id="symbol-101"></a>`interface RightAdd<R,T>` | 右操作数的加法重载接口。 |
| 模块 | <a id="symbol-102"></a>`interface Subtract<R,T>` | 左操作数的减法重载接口。 |
| 模块 | <a id="symbol-103"></a>`interface RightSubtract<R,T>` | 右操作数的减法重载接口。 |
| 模块 | <a id="symbol-104"></a>`interface Multiply<R,T>` | 左操作数的乘法重载接口。 |
| 模块 | <a id="symbol-105"></a>`interface RightMultiply<R,T>` | 右操作数的乘法重载接口。 |
| 模块 | <a id="symbol-106"></a>`interface Divide<R,T>` | 左操作数的除法重载接口。 |
| 模块 | <a id="symbol-107"></a>`interface RightDivide<R,T>` | 右操作数的除法重载接口。 |
| 模块 | <a id="symbol-108"></a>`interface Modulo<R,T>` | 左操作数的取模重载接口。 |
| 模块 | <a id="symbol-109"></a>`interface RightModulo<R,T>` | 右操作数的取模重载接口。 |
| 模块 | <a id="symbol-110"></a>`interface Equal<R,T>` | 左操作数的相等比较重载接口。 |
| 模块 | <a id="symbol-111"></a>`interface RightEqual<R,T>` | 右操作数的相等比较重载接口。 |
| 模块 | <a id="symbol-112"></a>`interface NotEqual<R,T>` | 左操作数的不等比较重载接口。 |
| 模块 | <a id="symbol-113"></a>`interface RightNotEqual<R,T>` | 右操作数的不等比较重载接口。 |
| 模块 | <a id="symbol-114"></a>`interface Less<R,T>` | 左操作数的小于比较重载接口。 |
| 模块 | <a id="symbol-115"></a>`interface RightLess<R,T>` | 右操作数的小于比较重载接口。 |
| 模块 | <a id="symbol-116"></a>`interface LessOrEqual<R,T>` | 左操作数的小于等于比较重载接口。 |
| 模块 | <a id="symbol-117"></a>`interface RightLessOrEqual<R,T>` | 右操作数的小于等于比较重载接口。 |
| 模块 | <a id="symbol-118"></a>`interface Greater<R,T>` | 左操作数的大于比较重载接口。 |
| 模块 | <a id="symbol-119"></a>`interface RightGreater<R,T>` | 右操作数的大于比较重载接口。 |
| 模块 | <a id="symbol-120"></a>`interface GreaterOrEqual<R,T>` | 左操作数的大于等于比较重载接口。 |
| 模块 | <a id="symbol-121"></a>`interface RightGreaterOrEqual<R,T>` | 右操作数的大于等于比较重载接口。 |
| 模块 | <a id="symbol-122"></a>`interface And<R,T>` | 左操作数的按位与重载接口。 |
| 模块 | <a id="symbol-123"></a>`interface RightAnd<R,T>` | 右操作数的按位与重载接口。 |
| 模块 | <a id="symbol-124"></a>`interface Or<R,T>` | 左操作数的按位或重载接口。 |
| 模块 | <a id="symbol-125"></a>`interface RightOr<R,T>` | 右操作数的按位或重载接口。 |
| 模块 | <a id="symbol-126"></a>`interface Xor<R,T>` | 左操作数的按位异或重载接口。 |
| 模块 | <a id="symbol-127"></a>`interface RightXor<R,T>` | 右操作数的按位异或重载接口。 |
| 模块 | <a id="symbol-128"></a>`interface ShiftLeft<R,T>` | 左操作数的左移重载接口。 |
| 模块 | <a id="symbol-129"></a>`interface RightShiftLeft<R,T>` | 右操作数的左移重载接口。 |
| 模块 | <a id="symbol-130"></a>`interface ShiftRight<R,T>` | 左操作数的右移重载接口。 |
| 模块 | <a id="symbol-131"></a>`interface RightShiftRight<R,T>` | 右操作数的右移重载接口。 |
| 模块 | <a id="symbol-132"></a>`interface Negate<R,T>` | 左操作数的取负重载接口。 |
| 模块 | <a id="symbol-133"></a>`interface BitNot<R,T>` | 左操作数的按位取反重载接口。 |
| 模块 | <a id="symbol-134"></a>`interface Positive<R,T>` | 左操作数的取正重载接口。 |
| 模块 | <a id="symbol-135"></a>`interface Hash<T>` | 为值提供哈希计算的接口。 |
| 模块 | <a id="symbol-136"></a>`interface Copy<T>` | 显式独立值复制协议，不改变赋值规则。 |
| 模块 | <a id="symbol-137"></a>`interface ExplicitConvert<T>` | 显式类型转换的接口。 |
| 模块 | <a id="symbol-138"></a>`interface ImplicitConvert<T>` | 隐式类型转换的接口。 |
| 模块 | <a id="symbol-139"></a>`interface Index<R,T>` | 自定义下标访问接口；与小写 index 连续存储函数区分。 |
| 模块 | <a id="symbol-140"></a>`interface Iterator<R>` | 供 for-in 获取和推进迭代状态的接口。 |

## 函数索引

| 名称 | 声明 |
| --- | --- |
| [rawSize](#api-0) | `int64 rawSize<T>()` |
| [String::length](#api-1) | `int64 length()` |
| [String::size](#api-2) | `int64 size()` |
| [String::byteLength](#api-3) | `int64 byteLength()` |
| [String::capacity](#api-4) | `int64 capacity()` |
| [String::maxCapacity](#api-5) | `int64 maxCapacity()` |
| [String::reserve](#api-6) | `void reserve(int64 bytes)` |
| [String::append](#api-7) | `void append(String text)` |
| [String::clear](#api-8) | `void clear()` |
| [String::truncate](#api-9) | `void truncate(int64 bytes)` |
| [String::isEmpty](#api-10) | `bool isEmpty()` |
| [String::isBlank](#api-11) | `bool isBlank()` |
| [String::isDigit](#api-12) | `bool isDigit()` |
| [String::isAlpha](#api-13) | `bool isAlpha()` |
| [String::isAlnum](#api-14) | `bool isAlnum()` |
| [String::isAscii](#api-15) | `bool isAscii()` |
| [String::charLength](#api-16) | `int64 charLength()` |
| [String::indexOf](#api-17) | `int64 indexOf(String text, int64 start = 0)` |
| [String::lastIndexOf](#api-18) | `int64 lastIndexOf(String text, int64 start = 0 - 1)` |
| [String::contains](#api-19) | `bool contains(String text)` |
| [String::startsWith](#api-20) | `bool startsWith(String text)` |
| [String::endsWith](#api-21) | `bool endsWith(String text)` |
| [String::count](#api-22) | `int64 count(String text)` |
| [String::compare](#api-23) | `int64 compare(String text)` |
| [String::byteAt](#api-24) | `Option<uint8> byteAt(int64 index)` |
| [String::substring](#api-25) | `String substring(int64 start, int64 count = 0 - 1)` |
| [String::slice](#api-26) | `String slice(int64 start, int64 end)` |
| [String::left](#api-27) | `String left(int64 count)` |
| [String::right](#api-28) | `String right(int64 count)` |
| [String::trim](#api-29) | `String trim()` |
| [String::trimStart](#api-30) | `String trimStart()` |
| [String::trimEnd](#api-31) | `String trimEnd()` |
| [String::toLower](#api-32) | `String toLower()` |
| [String::toUpper](#api-33) | `String toUpper()` |
| [String::replaceFirst](#api-34) | `String replaceFirst(String before, String after)` |
| [String::replaceAll](#api-35) | `String replaceAll(String before, String after)` |
| [String::replace](#api-36) | `String replace(String before, String after)` |
| [String::repeat](#api-37) | `String repeat(int64 count)` |
| [String::reverse](#api-38) | `String reverse()` |
| [String::padStart](#api-39) | `String padStart(int64 length, String fill = " ")` |
| [String::padEnd](#api-40) | `String padEnd(int64 length, String fill = " ")` |
| [String::removePrefix](#api-41) | `String removePrefix(String prefix)` |
| [String::removeSuffix](#api-42) | `String removeSuffix(String suffix)` |
| [String::insert](#api-43) | `String insert(int64 index, String text)` |
| [String::remove](#api-44) | `String remove(int64 start, int64 count = 0 - 1)` |
| [String::utf8Substring](#api-45) | `String utf8Substring(int64 start, int64 count = 0 - 1)` |
| [String::charAt](#api-46) | `Option<String> charAt(int64 index)` |
| [String::split](#api-47) | `List<String> split(String separator, bool keepEmpty = true)` |
| [String::splitWhitespace](#api-48) | `List<String> splitWhitespace()` |
| [String::lines](#api-49) | `List<String> lines(bool keepEmpty = true)` |
| [String::unicodeChars](#api-50) | `List<String> unicodeChars()` |
| [String::bytes](#api-51) | `List<uint8> bytes()` |
| [String::join](#api-52) | `String join(List<String> items)` |
| [String::concat](#api-53) | `String concat(String text)` |
| [String::prepend](#api-54) | `String prepend(String text)` |
| [Debug::debug](#api-55) | `String debug()` |
| [Error::toString](#api-56) | `String toString()` |
| [MessageError::toString](#api-57) | `String toString()` |
| [MessageError::from](#api-58) | `static MessageError from(String text)` |
| [File::print](#api-59) | `int64 print(String text)` |
| [File::println](#api-60) | `int64 println(String text = "")` |
| [readLine](#api-61) | `String readLine()` |
| [readLines](#api-62) | `String readLines(int32 N)` |
| [readLines](#api-63) | `String readLines(int64 N)` |
| [toInt32](#api-64) | `Option<int32> toInt32(float64 value)` |
| [toInt32](#api-65) | `Option<int32> toInt32(float32 value)` |
| [toInt64](#api-66) | `int64 toInt64(float64 value)` |
| [toInt64](#api-67) | `int64 toInt64(float32 value)` |
| [float32ToBytes](#api-68) | `List<uint8> float32ToBytes(float32 value, bool bigEndian)` |
| [float32ToBytes](#api-69) | `List<uint8> float32ToBytes(float32 value)` |
| [bytesToFloat32](#api-70) | `Option<float32> bytesToFloat32(List<uint8> bytes, bool bigEndian)` |
| [bytesToFloat32](#api-71) | `Option<float32> bytesToFloat32(List<uint8> bytes)` |
| [bytesToFloat32](#api-72) | `Option<float32> bytesToFloat32(uint8 const& bytes, int64 count, bool bigEndian)` |
| [bytesToFloat32](#api-73) | `Option<float32> bytesToFloat32(uint8 const& bytes, int64 count)` |
| [bytesToFloat32](#api-74) | `Option<float32> bytesToFloat32(uint8 bytes[], bool bigEndian)` |
| [bytesToFloat32](#api-75) | `Option<float32> bytesToFloat32(uint8 bytes[])` |
| [float32ToBytes](#api-76) | `bool float32ToBytes(float32 value, uint8 mut& bytes, int64 count, bool bigEndian)` |
| [float32ToBytes](#api-77) | `bool float32ToBytes(float32 value, uint8 mut& bytes, int64 count)` |
| [float32ToBytes](#api-78) | `bool float32ToBytes(float32 value, uint8[] mut& bytes, bool bigEndian)` |
| [float32ToBytes](#api-79) | `bool float32ToBytes(float32 value, uint8[] mut& bytes)` |
| [float64ToBytes](#api-80) | `List<uint8> float64ToBytes(float64 value, bool bigEndian)` |
| [float64ToBytes](#api-81) | `List<uint8> float64ToBytes(float64 value)` |
| [bytesToFloat64](#api-82) | `Option<float64> bytesToFloat64(List<uint8> bytes, bool bigEndian)` |
| [bytesToFloat64](#api-83) | `Option<float64> bytesToFloat64(List<uint8> bytes)` |
| [bytesToFloat64](#api-84) | `Option<float64> bytesToFloat64(uint8 const& bytes, int64 count, bool bigEndian)` |
| [bytesToFloat64](#api-85) | `Option<float64> bytesToFloat64(uint8 const& bytes, int64 count)` |
| [bytesToFloat64](#api-86) | `Option<float64> bytesToFloat64(uint8 bytes[], bool bigEndian)` |
| [bytesToFloat64](#api-87) | `Option<float64> bytesToFloat64(uint8 bytes[])` |
| [float64ToBytes](#api-88) | `bool float64ToBytes(float64 value, uint8 mut& bytes, int64 count, bool bigEndian)` |
| [float64ToBytes](#api-89) | `bool float64ToBytes(float64 value, uint8 mut& bytes, int64 count)` |
| [float64ToBytes](#api-90) | `bool float64ToBytes(float64 value, uint8[] mut& bytes, bool bigEndian)` |
| [float64ToBytes](#api-91) | `bool float64ToBytes(float64 value, uint8[] mut& bytes)` |
| [assert](#api-92) | `void assert(bool condition)` |
| [assert](#api-93) | `void assert(bool condition, String message)` |
| [assert](#api-94) | `void assert(bool condition, int64 line)` |
| [assert](#api-95) | `void assert(bool condition, String message, int64 line)` |
| [Try::branch](#api-96) | `TryBranch<T, R> branch()` |
| [TryMove::intoBranch](#api-97) | `TryBranch<T, R> intoBranch()` |
| [FromResidual::fromResidual](#api-98) | `static F fromResidual(R residual)` |
| [Option::branch](#api-99) | `TryBranch<T, NoneResidual> branch()` |
| [Option::intoBranch](#api-100) | `TryBranch<T, NoneResidual> intoBranch()` |
| [Option::fromResidual](#api-101) | `static Option<T> fromResidual(NoneResidual residual)` |
| [Option::isValue](#api-102) | `bool isValue()` |
| [Option::isNone](#api-103) | `bool isNone()` |
| [Option::take](#api-104) | `T take()` |
| [Option::unwrap](#api-105) | `T unwrap()` |
| [Option::unwrapOr](#api-106) | `T unwrapOr(T fallback)` |
| [Option::expect](#api-107) | `T expect(String message)` |
| [Option::transform](#api-108) | `Option<T> transform(T fn(T) mapper)` |
| [Option::andThen](#api-109) | `Option<T> andThen(Option<T> fn(T) mapper)` |
| [Result::branch](#api-110) | `TryBranch<T, ErrorResidual<E>> branch()` |
| [Result::intoBranch](#api-111) | `TryBranch<T, ErrorResidual<E>> intoBranch()` |
| [Result::fromResidual](#api-112) | `static Result<T,E> fromResidual(ErrorResidual<E> residual)` |
| [Result::ok](#api-113) | `Option<T> ok()` |
| [Result::isOk](#api-114) | `bool isOk()` |
| [Result::isErr](#api-115) | `bool isErr()` |
| [Result::take](#api-116) | `T take()` |
| [Result::unwrap](#api-117) | `T unwrap()` |
| [Result::unwrapOr](#api-118) | `T unwrapOr(T fallback)` |
| [Result::expect](#api-119) | `T expect(String message)` |
| [Result::takeError](#api-120) | `E takeError()` |
| [Result::error](#api-121) | `E error()` |
| [Result::transform](#api-122) | `Result<T, E> transform(T fn(T) mapper)` |
| [Result::andThen](#api-123) | `Result<T, E> andThen(Result<T, E> fn(T) mapper)` |
| [NextValue::nextValue](#api-124) | `Option<T> nextValue(T end, bool inclusive)` |
| [Pair::create](#api-125) | `static Pair<K, V> create(K first, V second)` |
| [Pair::create](#api-126) | `static Pair<K, V> create((K, V) value)` |
| [Pair::toTuple](#api-127) | `(K, V) toTuple()` |
| [isType](#api-128) | `bool isType<T>(Any const& value)` |
| [tryGet](#api-129) | `Option<T> tryGet<T>(Any const& value)` |
| [tryGetReference](#api-130) | `Option<T const&> tryGetReference<T>(Any const& value)` |
| [tryGetMut](#api-131) | `Option<T mut&> tryGetMut<T>(Any mut& value)` |
| [stringBytes](#api-132) | `List<uint8> stringBytes(String text)` |
| [BarePtr::leftAdd](#api-133) | `BarePtr<T> leftAdd(int64 elements)` |
| [BarePtr::rightAdd](#api-134) | `BarePtr<T> rightAdd(int64 elements)` |
| [BarePtr::leftSub](#api-135) | `BarePtr<T> leftSub(int64 elements)` |
| [BarePtr::leftSub](#api-136) | `int64 leftSub(BarePtr<T> other)` |
| [barePointer](#api-137) | `unsafe BarePtr<T> barePointer<T>(RawPtr<T> pointer)` |
| [pointerFromAddress](#api-138) | `unsafe BarePtr<T> pointerFromAddress<T>(int64 address)` |
| [dereference](#api-139) | `unsafe T mut& dereference<T>(BarePtr<T> pointer)` |
| [memcpy](#api-140) | `unsafe void memcpy<T,U>(BarePtr<T> destination, BarePtr<U> source, int64 bytes)` |
| [memset](#api-141) | `unsafe void memset<T>(BarePtr<T> destination, uint8 value, int64 bytes)` |
| [initAt](#api-142) | `unsafe void initAt<T>(RawPtr<T> pointer, T value)` |
| [readAt](#api-143) | `unsafe T readAt<T>(RawPtr<T> pointer)` |
| [borrowAt](#api-144) | `unsafe T mut& borrowAt<T>(RawPtr<T> pointer)` |
| [withBorrowAt](#api-145) | `unsafe void withBorrowAt<T>(RawPtr<T> pointer, void fn(T mut&) use)` |
| [takeAt](#api-146) | `unsafe T takeAt<T>(RawPtr<T> pointer)` |
| [dropAt](#api-147) | `unsafe void dropAt<T>(RawPtr<T> pointer)` |
| [relocateNonOverlapping](#api-148) | `unsafe void relocateNonOverlapping<T>(RawPtr<T> destination, RawPtr<T> source, int64 count)` |
| [RawStorage::allocate](#api-149) | `static RawStorage<T> allocate(int64 capacity)` |
| [RawStorage::capacity](#api-150) | `int64 capacity()` |
| [RawStorage::pointer](#api-151) | `RawPtr<T> pointer(int64 index)` |
| [RawStorage::isInitialized](#api-152) | `bool isInitialized(int64 index)` |
| [RawStorage::close](#api-153) | `void close()` |
| [Box::newBox](#api-154) | `static Box<T> newBox(T value)` |
| [Box::isEmpty](#api-155) | `bool isEmpty()` |
| [Box::get](#api-156) | `T get()` |
| [Box::take](#api-157) | `T take()` |
| [Box::close](#api-158) | `void close()` |
| [range](#api-159) | `__Range range(int64 start, int64 stop)` |
| [range](#api-160) | `__Range range(int64 start, int64 stop, int64 step)` |
| [rangeEq](#api-161) | `__Interval_int64 rangeEq(int64 start, int64 stop)` |
| [Drop::drop](#api-162) | `void drop()` |
| [Closeable::close](#api-163) | `void close()` |
| [print](#api-164) | `int64 print(String format, Any args...)` |
| [println](#api-165) | `int64 println(String format, Any args...)` |
| [eprint](#api-166) | `int64 eprint(String format, Any args...)` |
| [eprintln](#api-167) | `int64 eprintln(String format, Any args...)` |
| [format](#api-168) | `String format(String format, Any args...)` |
| [putChar](#api-169) | `int64 putChar(char value)` |
| [pause](#api-170) | `int64 pause()` |
| [inputLine](#api-171) | `String inputLine()` |
| [inputInt](#api-172) | `int64 inputInt()` |
| [inputUInt](#api-173) | `uint64 inputUInt()` |
| [inputFloat](#api-174) | `float64 inputFloat()` |
| [inputBool](#api-175) | `bool inputBool()` |
| [inputChar](#api-176) | `char inputChar()` |
| [inputBigInteger](#api-177) | `BigInteger inputBigInteger()` |
| [inputBigDecimal](#api-178) | `BigDecimal inputBigDecimal()` |
| [panic](#api-179) | `int64 panic(String message)` |
| [len](#api-180) | `int64 len(Any value)` |
| [toReference](#api-181) | `T mut& toReference<T>(Option<T mut&> value)` |
| [toOptional](#api-182) | `Option<T mut&> toOptional<T>(T mut& value)` |
| [memcpy](#api-183) | `int64 memcpy(char mut& destination, char const& source, int64 bytes)` |
| [memset](#api-184) | `int64 memset(char mut& destination, int64 value, int64 bytes)` |
| [List::copy](#api-185) | `List<T> copy()` |
| [List::push](#api-186) | `int64 push(T value)` |
| [List::insert](#api-187) | `int64 insert(int64 index, T value)` |
| [List::at](#api-188) | `T at(int64 index)` |
| [List::pop](#api-189) | `T pop()` |
| [List::remove](#api-190) | `int64 remove(int64 index)` |
| [List::indexOf](#api-191) | `int64 indexOf(T value)` |
| [List::contains](#api-192) | `bool contains(T value)` |
| [List::has](#api-193) | `bool has(T value)` |
| [List::clear](#api-194) | `int64 clear()` |
| [List::len](#api-195) | `int64 len()` |
| [Set::copy](#api-196) | `Set<T> copy()` |
| [Set::add](#api-197) | `int64 add(T value)` |
| [Set::has](#api-198) | `int64 has(T value)` |
| [Set::contains](#api-199) | `int64 contains(T value)` |
| [Set::remove](#api-200) | `int64 remove(T value)` |
| [Set::at](#api-201) | `T at(int64 index)` |
| [Set::clear](#api-202) | `int64 clear()` |
| [Set::len](#api-203) | `int64 len()` |
| [Map::copy](#api-204) | `Map<K,V> copy()` |
| [Map::put](#api-205) | `int64 put(K key,V value)` |
| [Map::has](#api-206) | `int64 has(K key)` |
| [Map::contains](#api-207) | `int64 contains(K key)` |
| [Map::remove](#api-208) | `int64 remove(K key)` |
| [Map::keyAt](#api-209) | `K keyAt(int64 index)` |
| [Map::valAt](#api-210) | `V valAt(int64 index)` |
| [Map::clear](#api-211) | `int64 clear()` |
| [Map::len](#api-212) | `int64 len()` |
| [Any::copy](#api-213) | `Any copy()` |
| [Any::isEmpty](#api-214) | `bool isEmpty()` |
| [Any::clear](#api-215) | `void clear()` |
| [Any::set](#api-216) | `int64 set(T value)` |
| [Any::getValue](#api-217) | `T getValue()` |
| [Any::getReference](#api-218) | `T mut& getReference()` |
| [Any::getOptional](#api-219) | `Option<T mut&> getOptional()` |
| [eq](#api-220) | `bool eq(T left,T right)` |
| [ne](#api-221) | `bool ne(T left,T right)` |
| [lt](#api-222) | `bool lt(T left,T right)` |
| [le](#api-223) | `bool le(T left,T right)` |
| [gt](#api-224) | `bool gt(T left,T right)` |
| [ge](#api-225) | `bool ge(T left,T right)` |
| [add](#api-226) | `R add(T left,U right)` |
| [sub](#api-227) | `R sub(T left,U right)` |
| [mul](#api-228) | `R mul(T left,U right)` |
| [div](#api-229) | `R div(T left,U right)` |
| [mod](#api-230) | `R mod(T left,U right)` |
| [index](#api-231) | `T index(T const& data,int64 index)` |
| [Add::leftAdd](#api-232) | `R leftAdd(T value)` |
| [RightAdd::rightAdd](#api-233) | `R rightAdd(T value)` |
| [Subtract::leftSub](#api-234) | `R leftSub(T value)` |
| [RightSubtract::rightSub](#api-235) | `R rightSub(T value)` |
| [Multiply::leftMul](#api-236) | `R leftMul(T value)` |
| [RightMultiply::rightMul](#api-237) | `R rightMul(T value)` |
| [Divide::leftDiv](#api-238) | `R leftDiv(T value)` |
| [RightDivide::rightDiv](#api-239) | `R rightDiv(T value)` |
| [Modulo::leftMod](#api-240) | `R leftMod(T value)` |
| [RightModulo::rightMod](#api-241) | `R rightMod(T value)` |
| [Equal::leftEqual](#api-242) | `R leftEqual(T value)` |
| [RightEqual::rightEqual](#api-243) | `R rightEqual(T value)` |
| [NotEqual::leftNotEqual](#api-244) | `R leftNotEqual(T value)` |
| [RightNotEqual::rightNotEqual](#api-245) | `R rightNotEqual(T value)` |
| [Less::leftLess](#api-246) | `R leftLess(T value)` |
| [RightLess::rightLess](#api-247) | `R rightLess(T value)` |
| [LessOrEqual::leftLessEqual](#api-248) | `R leftLessEqual(T value)` |
| [RightLessOrEqual::rightLessEqual](#api-249) | `R rightLessEqual(T value)` |
| [Greater::leftGreater](#api-250) | `R leftGreater(T value)` |
| [RightGreater::rightGreater](#api-251) | `R rightGreater(T value)` |
| [GreaterOrEqual::leftGreaterEqual](#api-252) | `R leftGreaterEqual(T value)` |
| [RightGreaterOrEqual::rightGreaterEqual](#api-253) | `R rightGreaterEqual(T value)` |
| [And::leftBitAnd](#api-254) | `R leftBitAnd(T value)` |
| [RightAnd::rightBitAnd](#api-255) | `R rightBitAnd(T value)` |
| [Or::leftBitOr](#api-256) | `R leftBitOr(T value)` |
| [RightOr::rightBitOr](#api-257) | `R rightBitOr(T value)` |
| [Xor::leftBitXor](#api-258) | `R leftBitXor(T value)` |
| [RightXor::rightBitXor](#api-259) | `R rightBitXor(T value)` |
| [ShiftLeft::leftShiftLeft](#api-260) | `R leftShiftLeft(T value)` |
| [RightShiftLeft::rightShiftLeft](#api-261) | `R rightShiftLeft(T value)` |
| [ShiftRight::leftShiftRight](#api-262) | `R leftShiftRight(T value)` |
| [RightShiftRight::rightShiftRight](#api-263) | `R rightShiftRight(T value)` |
| [Negate::negate](#api-264) | `R negate(T value)` |
| [BitNot::bitNot](#api-265) | `R bitNot(T value)` |
| [Positive::positive](#api-266) | `R positive(T value)` |
| [Hash::hash](#api-267) | `uint64 hash(T value)` |
| [Copy::copy](#api-268) | `T copy()` |
| [ExplicitConvert::explicitConvert](#api-269) | `T explicitConvert()` |
| [ImplicitConvert::implicitConvert](#api-270) | `T implicitConvert()` |
| [Index::index](#api-271) | `R index(T key)` |
| [Iterator::iterator](#api-272) | `Option<R> iterator(int64 index)` |
| [Result::mapErr](#api-273) | `Result<T,F> mapErr<F>(F fn(E) mapper)` |
| [Option::okOr](#api-274) | `Result<T,E> okOr<E>(E error)` |
| [Option::okOrElse](#api-275) | `Result<T,E> okOrElse<E>(E fn() factory)` |
| [BarePtr::castTo](#api-276) | `unsafe BarePtr<U> castTo<U>()` |

<a id="api-0"></a>

### rawSize（1）

**声明**

```lepus
int64 rawSize<T>();
```

**行为**

查询 T 在当前 Lepus 值存储中的字节数；不访问对象，也不返回动态分配占用。 uint8 与 char 使用一个字节，其余普通值槽为八字节；聚合按语言布局计算，不等于宿主 C ABI 的 sizeof。 无需 unsafe；不支持 void、直接数组或宿主别名，数组可放入具名结构体后查询。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`int64`。查询 T 在当前 Lepus 值存储中的字节数；不访问对象，也不返回动态分配占用。 uint8 与 char 使用一个字节，其余普通值槽为八字节；聚合按语言布局计算，不等于宿主 C ABI 的 sizeof。 无需 unsafe；不支持 void、直接数组或宿主别名，数组可放入具名结构体后查询。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
rawSize<T>();
```

**源码**：[source/std/core.lep:8](../../source/std/core.lep#L8)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-1"></a>

### String::length（2）

**声明**

```lepus
int64 length();
```

**行为**

返回 UTF-8 文本的字节长度。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`int64`。不包括终止零字节的长度。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.length();
```

**源码**：[source/std/core.lep:567](../../source/std/core.lep#L567)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-2"></a>

### String::size（3）

**声明**

```lepus
int64 size();
```

**行为**

返回存储的总长度。

返回 UTF-8 字节数，与 length 一致。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`int64`。返回存储的总长度。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.size();
```

**源码**：[source/std/core.lep:569](../../source/std/core.lep#L569)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-3"></a>

### String::byteLength（4）

**声明**

```lepus
int64 byteLength();
```

**行为**

返回字节数，不包含终止零字节。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`int64`。返回字节数，不包含终止零字节。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.byteLength();
```

**源码**：[source/std/core.lep:572](../../source/std/core.lep#L572)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-4"></a>

### String::capacity（5）

**声明**

```lepus
int64 capacity();
```

**行为**

当前可复用的字节容量，不包含终止零字节。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`int64`。当前可复用的字节容量，不包含终止零字节。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.capacity();
```

**源码**：[source/std/core.lep:575](../../source/std/core.lep#L575)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-5"></a>

### String::maxCapacity（6）

**声明**

```lepus
int64 maxCapacity();
```

**行为**

分配器允许的最大缓冲区容量，其他存活内存仍可能使申请失败。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`int64`。分配器允许的最大缓冲区容量，其他存活内存仍可能使申请失败。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.maxCapacity();
```

**源码**：[source/std/core.lep:578](../../source/std/core.lep#L578)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-6"></a>

### String::reserve（7）

**声明**

```lepus
void reserve(int64 bytes);
```

**行为**

预留至少 bytes 字节；不足时几何增长，已有内容保持不变。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `bytes` | `int64 bytes` | 输入或输出字节存储 | 无 |

**返回值**

无返回值。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.reserve(bytes);
```

**源码**：[source/std/core.lep:581](../../source/std/core.lep#L581)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-7"></a>

### String::append（8）

**声明**

```lepus
void append(String text);
```

**行为**

原地追加文本字节，保留容量并允许向自身追加。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `text` | `String text` | 输入文本，位置与长度通常按字节 | 无 |

**返回值**

无返回值。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.append(text);
```

**源码**：[source/std/core.lep:584](../../source/std/core.lep#L584)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-8"></a>

### String::clear（9）

**声明**

```lepus
void clear();
```

**行为**

移除全部元素。

清空有效字节，保留容量供后续追加复用。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

无返回值。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.clear();
```

**源码**：[source/std/core.lep:587](../../source/std/core.lep#L587)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-9"></a>

### String::truncate（10）

**声明**

```lepus
void truncate(int64 bytes);
```

**行为**

按字节截短，超出现有长度时保持原值；保留容量，不进行 UTF-8 重编码。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `bytes` | `int64 bytes` | 输入或输出字节存储 | 无 |

**返回值**

无返回值。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.truncate(bytes);
```

**源码**：[source/std/core.lep:590](../../source/std/core.lep#L590)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-10"></a>

### String::isEmpty（11）

**声明**

```lepus
bool isEmpty();
```

**行为**

判断 Any 是否为空盒子，不公开内部执行期标签。

是否为空字符串。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`bool`。判断 Any 是否为空盒子，不公开内部执行期标签。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.isEmpty();
```

**源码**：[source/std/core.lep:593](../../source/std/core.lep#L593)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-11"></a>

### String::isBlank（12）

**声明**

```lepus
bool isBlank();
```

**行为**

是否仅含 ASCII 空白；空字符串也返回 true。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`bool`。是否仅含 ASCII 空白；空字符串也返回 true。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.isBlank();
```

**源码**：[source/std/core.lep:596](../../source/std/core.lep#L596)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-12"></a>

### String::isDigit（13）

**声明**

```lepus
bool isDigit();
```

**行为**

是否为非空 ASCII 十进制数字串。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`bool`。是否为非空 ASCII 十进制数字串。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.isDigit();
```

**源码**：[source/std/core.lep:599](../../source/std/core.lep#L599)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-13"></a>

### String::isAlpha（14）

**声明**

```lepus
bool isAlpha();
```

**行为**

是否为非空 ASCII 字母串。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`bool`。是否为非空 ASCII 字母串。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.isAlpha();
```

**源码**：[source/std/core.lep:602](../../source/std/core.lep#L602)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-14"></a>

### String::isAlnum（15）

**声明**

```lepus
bool isAlnum();
```

**行为**

是否为非空 ASCII 字母数字串。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`bool`。是否为非空 ASCII 字母数字串。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.isAlnum();
```

**源码**：[source/std/core.lep:605](../../source/std/core.lep#L605)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-15"></a>

### String::isAscii（16）

**声明**

```lepus
bool isAscii();
```

**行为**

所有字节是否均小于 128；空串返回 true。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`bool`。所有字节是否均小于 128；空串返回 true。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.isAscii();
```

**源码**：[source/std/core.lep:608](../../source/std/core.lep#L608)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-16"></a>

### String::charLength（17）

**声明**

```lepus
int64 charLength();
```

**行为**

严格解码 UTF-8 后的 Unicode 码点数，非字素簇数。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`int64`。严格解码 UTF-8 后的 Unicode 码点数，非字素簇数。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.charLength();
```

**源码**：[source/std/core.lep:611](../../source/std/core.lep#L611)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-17"></a>

### String::indexOf（18）

**声明**

```lepus
int64 indexOf(String text, int64 start = 0);
```

**行为**

返回首次出现的下标；未找到返回 -1。

从 start 字节起搜索，未找到返回 -1；负起点按零处理。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `text` | `String text` | 输入文本，位置与长度通常按字节 | 无 |
| `start` | `int64 start` | 起始值 | `0` |

**返回值**

`int64`。返回首次出现的下标；未找到返回 -1。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.indexOf(text, start);
```

**源码**：[source/std/core.lep:614](../../source/std/core.lep#L614)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-18"></a>

### String::lastIndexOf（19）

**声明**

```lepus
int64 lastIndexOf(String text, int64 start = 0 - 1);
```

**行为**

从 start 字节向前搜索，负起点表示从末尾开始。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `text` | `String text` | 输入文本，位置与长度通常按字节 | 无 |
| `start` | `int64 start` | 起始值 | `0 - 1` |

**返回值**

`int64`。从 start 字节向前搜索，负起点表示从末尾开始。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.lastIndexOf(text, start);
```

**源码**：[source/std/core.lep:617](../../source/std/core.lep#L617)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-19"></a>

### String::contains（20）

**声明**

```lepus
bool contains(String text);
```

**行为**

判断是否包含目标值或匹配文本。

是否包含指定字节序列；空搜索串总是匹配。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `text` | `String text` | 输入文本，位置与长度通常按字节 | 无 |

**返回值**

`bool`。判断是否包含目标值或匹配文本。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.contains(text);
```

**源码**：[source/std/core.lep:620](../../source/std/core.lep#L620)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-20"></a>

### String::startsWith（21）

**声明**

```lepus
bool startsWith(String text);
```

**行为**

判断文本是否以 prefix 开始。

是否以指定文本开头；空前缀总是匹配。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `text` | `String text` | 输入文本，位置与长度通常按字节 | 无 |

**返回值**

`bool`。判断文本是否以 prefix 开始。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.startsWith(text);
```

**源码**：[source/std/core.lep:623](../../source/std/core.lep#L623)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-21"></a>

### String::endsWith（22）

**声明**

```lepus
bool endsWith(String text);
```

**行为**

判断文本是否以 suffix 结束。

是否以指定文本结尾；空后缀总是匹配。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `text` | `String text` | 输入文本，位置与长度通常按字节 | 无 |

**返回值**

`bool`。判断文本是否以 suffix 结束。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.endsWith(text);
```

**源码**：[source/std/core.lep:626](../../source/std/core.lep#L626)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-22"></a>

### String::count（23）

**声明**

```lepus
int64 count(String text);
```

**行为**

读取结果首行首列的整数表示。

统计不重叠匹配数；空搜索串返回字节数加一。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `text` | `String text` | 输入文本，位置与长度通常按字节 | 无 |

**返回值**

`int64`。读取结果首行首列的整数表示。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.count(text);
```

**源码**：[source/std/core.lep:629](../../source/std/core.lep#L629)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-23"></a>

### String::compare（24）

**声明**

```lepus
int64 compare(String text);
```

**行为**

按字典序比较，返回 -1、0 或 1。

按字节字典序比较，返回 -1、0 或 1。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `text` | `String text` | 输入文本，位置与长度通常按字节 | 无 |

**返回值**

`int64`。按字典序比较，返回 -1、0 或 1。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.compare(text);
```

**源码**：[source/std/core.lep:632](../../source/std/core.lep#L632)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-24"></a>

### String::byteAt（25）

**声明**

```lepus
Option<uint8> byteAt(int64 index);
```

**行为**

读取字节，越界返回 None。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `index` | `int64 index` | 从 0 开始的位置 | 无 |

**返回值**

`Option<uint8>`。读取字节，越界返回 None。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.byteAt(index);
```

**源码**：[source/std/core.lep:635](../../source/std/core.lep#L635)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-25"></a>

### String::substring（26）

**声明**

```lepus
String substring(int64 start, int64 count = 0 - 1);
```

**行为**

按字节起点及长度截取，越界截断，负起点按零，负长度表示至末尾。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `start` | `int64 start` | 起始值 | 无 |
| `count` | `int64 count` | 元素或字节数量，具体单位见行为说明 | `0 - 1` |

**返回值**

`String`。按字节起点及长度截取，越界截断，负起点按零，负长度表示至末尾。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.substring(start, count);
```

**源码**：[source/std/core.lep:638](../../source/std/core.lep#L638)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-26"></a>

### String::slice（27）

**声明**

```lepus
String slice(int64 start, int64 end);
```

**行为**

按半开字节范围截取；结束不大于起点时返回空串。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `start` | `int64 start` | 起始值 | 无 |
| `end` | `int64 end` | 积分终点 | 无 |

**返回值**

`String`。按半开字节范围截取；结束不大于起点时返回空串。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.slice(start, end);
```

**源码**：[source/std/core.lep:641](../../source/std/core.lep#L641)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-27"></a>

### String::left（28）

**声明**

```lepus
String left(int64 count);
```

**行为**

取前 count 字节，负数按零。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `count` | `int64 count` | 元素或字节数量，具体单位见行为说明 | 无 |

**返回值**

`String`。取前 count 字节，负数按零。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.left(count);
```

**源码**：[source/std/core.lep:644](../../source/std/core.lep#L644)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-28"></a>

### String::right（29）

**声明**

```lepus
String right(int64 count);
```

**行为**

取后 count 字节，负数按零，超出长度取全部。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `count` | `int64 count` | 元素或字节数量，具体单位见行为说明 | 无 |

**返回值**

`String`。取后 count 字节，负数按零，超出长度取全部。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.right(count);
```

**源码**：[source/std/core.lep:647](../../source/std/core.lep#L647)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-29"></a>

### String::trim（30）

**声明**

```lepus
String trim();
```

**行为**

去掉两端 ASCII 空白。

去除两端 ASCII 空白，保留内部文本。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`String`。去掉两端 ASCII 空白。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.trim();
```

**源码**：[source/std/core.lep:650](../../source/std/core.lep#L650)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-30"></a>

### String::trimStart（31）

**声明**

```lepus
String trimStart();
```

**行为**

去除起始 ASCII 空白。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`String`。去除起始 ASCII 空白。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.trimStart();
```

**源码**：[source/std/core.lep:653](../../source/std/core.lep#L653)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-31"></a>

### String::trimEnd（32）

**声明**

```lepus
String trimEnd();
```

**行为**

去除末尾 ASCII 空白。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`String`。去除末尾 ASCII 空白。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.trimEnd();
```

**源码**：[source/std/core.lep:656](../../source/std/core.lep#L656)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-32"></a>

### String::toLower（33）

**声明**

```lepus
String toLower();
```

**行为**

将 ASCII 大写字母转换成小写。

将 ASCII 大写字母转小写，其他字节保持原样。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`String`。将 ASCII 大写字母转换成小写。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.toLower();
```

**源码**：[source/std/core.lep:659](../../source/std/core.lep#L659)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-33"></a>

### String::toUpper（34）

**声明**

```lepus
String toUpper();
```

**行为**

将 ASCII 小写字母转换成大写。

将 ASCII 小写字母转大写，其他字节保持原样。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`String`。将 ASCII 小写字母转换成大写。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.toUpper();
```

**源码**：[source/std/core.lep:662](../../source/std/core.lep#L662)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-34"></a>

### String::replaceFirst（35）

**声明**

```lepus
String replaceFirst(String before, String after);
```

**行为**

替换首次正则匹配。

替换第一个精确匹配；空搜索串保持原值。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `before` | `String before` | 待替换的精确匹配文本 | 无 |
| `after` | `String after` | 替换文本 | 无 |

**返回值**

`String`。替换首次正则匹配。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.replaceFirst(before, after);
```

**源码**：[source/std/core.lep:665](../../source/std/core.lep#L665)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-35"></a>

### String::replaceAll（36）

**声明**

```lepus
String replaceAll(String before, String after);
```

**行为**

替换全部正则匹配。

替换全部不重叠精确匹配；空搜索串保持原值。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `before` | `String before` | 待替换的精确匹配文本 | 无 |
| `after` | `String after` | 替换文本 | 无 |

**返回值**

`String`。替换全部正则匹配。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.replaceAll(before, after);
```

**源码**：[source/std/core.lep:668](../../source/std/core.lep#L668)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-36"></a>

### String::replace（37）

**声明**

```lepus
String replace(String before, String after);
```

**行为**

将所有 from 子串替换为 to；空 from 保留副本。

替换全部不重叠匹配，与 replaceAll 一致。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `before` | `String before` | 待替换的精确匹配文本 | 无 |
| `after` | `String after` | 替换文本 | 无 |

**返回值**

`String`。将所有 from 子串替换为 to；空 from 保留副本。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.replace(before, after);
```

**源码**：[source/std/core.lep:671](../../source/std/core.lep#L671)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-37"></a>

### String::repeat（38）

**声明**

```lepus
String repeat(int64 count);
```

**行为**

重复给定值或文本；文本次数非正时为空串，迭代器版本为无限源。

重复 count 次，零次返回空串，负数报错。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `count` | `int64 count` | 元素或字节数量，具体单位见行为说明 | 无 |

**返回值**

`String`。重复给定值或文本；文本次数非正时为空串，迭代器版本为无限源。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.repeat(count);
```

**源码**：[source/std/core.lep:674](../../source/std/core.lep#L674)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-38"></a>

### String::reverse（39）

**声明**

```lepus
String reverse();
```

**行为**

按 Unicode 码点反转，保持每个 UTF-8 编码完整。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`String`。按 Unicode 码点反转，保持每个 UTF-8 编码完整。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.reverse();
```

**源码**：[source/std/core.lep:677](../../source/std/core.lep#L677)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-39"></a>

### String::padStart（40）

**声明**

```lepus
String padStart(int64 length, String fill = " ");
```

**行为**

在左侧以 fill 补到 width 字节。

按字节目标长度向左循环填充；空填充串保持原值。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `length` | `int64 length` | 目标有效元素数，必须非负。 | 无 |
| `fill` | `String fill` | 填充字节 | `" "` |

**返回值**

`String`。在左侧以 fill 补到 width 字节。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.padStart(length, fill);
```

**源码**：[source/std/core.lep:680](../../source/std/core.lep#L680)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-40"></a>

### String::padEnd（41）

**声明**

```lepus
String padEnd(int64 length, String fill = " ");
```

**行为**

在右侧以 fill 补到 width 字节。

按字节目标长度向右循环填充；空填充串保持原值。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `length` | `int64 length` | 目标有效元素数，必须非负。 | 无 |
| `fill` | `String fill` | 填充字节 | `" "` |

**返回值**

`String`。在右侧以 fill 补到 width 字节。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.padEnd(length, fill);
```

**源码**：[source/std/core.lep:683](../../source/std/core.lep#L683)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-41"></a>

### String::removePrefix（42）

**声明**

```lepus
String removePrefix(String prefix);
```

**行为**

仅去除匹配的起始前缀一次。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `prefix` | `String prefix` | 前缀 | 无 |

**返回值**

`String`。仅去除匹配的起始前缀一次。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.removePrefix(prefix);
```

**源码**：[source/std/core.lep:686](../../source/std/core.lep#L686)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-42"></a>

### String::removeSuffix（43）

**声明**

```lepus
String removeSuffix(String suffix);
```

**行为**

仅去除匹配的末尾后缀一次。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `suffix` | `String suffix` | 后缀 | 无 |

**返回值**

`String`。仅去除匹配的末尾后缀一次。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.removeSuffix(suffix);
```

**源码**：[source/std/core.lep:689](../../source/std/core.lep#L689)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-43"></a>

### String::insert（44）

**声明**

```lepus
String insert(int64 index, String text);
```

**行为**

在 index 之前插入 value。

在截断到有效范围的字节位置插入文本，返回新值。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `index` | `int64 index` | 从 0 开始的位置 | 无 |
| `text` | `String text` | 输入文本，位置与长度通常按字节 | 无 |

**返回值**

`String`。在 index 之前插入 value。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.insert(index, text);
```

**源码**：[source/std/core.lep:692](../../source/std/core.lep#L692)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-44"></a>

### String::remove（45）

**声明**

```lepus
String remove(int64 start, int64 count = 0 - 1);
```

**行为**

删除指定位置或键的元素。

移除指定字节段；负长度表示删除余下全部。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `start` | `int64 start` | 起始值 | 无 |
| `count` | `int64 count` | 元素或字节数量，具体单位见行为说明 | `0 - 1` |

**返回值**

`String`。删除指定位置或键的元素。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.remove(start, count);
```

**源码**：[source/std/core.lep:695](../../source/std/core.lep#L695)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-45"></a>

### String::utf8Substring（46）

**声明**

```lepus
String utf8Substring(int64 start, int64 count = 0 - 1);
```

**行为**

按 Unicode 码点截取，严格验证 UTF-8，不按字素簇处理。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `start` | `int64 start` | 起始值 | 无 |
| `count` | `int64 count` | 元素或字节数量，具体单位见行为说明 | `0 - 1` |

**返回值**

`String`。按 Unicode 码点截取，严格验证 UTF-8，不按字素簇处理。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.utf8Substring(start, count);
```

**源码**：[source/std/core.lep:698](../../source/std/core.lep#L698)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-46"></a>

### String::charAt（47）

**声明**

```lepus
Option<String> charAt(int64 index);
```

**行为**

读取 index 位置的字节，需处于有效 UTF-8 边界。

读取一个完整 Unicode 码点，越界返回 None。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `index` | `int64 index` | 从 0 开始的位置 | 无 |

**返回值**

`Option<String>`。读取 index 位置的字节，需处于有效 UTF-8 边界。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.charAt(index);
```

**源码**：[source/std/core.lep:701](../../source/std/core.lep#L701)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-47"></a>

### String::split（48）

**声明**

```lepus
List<String> split(String separator, bool keepEmpty = true);
```

**行为**

按 separator 分割，空分隔符把整串作为一项。

按精确分隔文本分割，默认保留空项；空分隔符按 UTF-8 码点分割。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `separator` | `String separator` | 分隔文本 | 无 |
| `keepEmpty` | `bool keepEmpty` | 是否保留分割产生的空项 | `true` |

**返回值**

`List<String>`。按 separator 分割，空分隔符把整串作为一项。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.split(separator, keepEmpty);
```

**源码**：[source/std/core.lep:704](../../source/std/core.lep#L704)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-48"></a>

### String::splitWhitespace（49）

**声明**

```lepus
List<String> splitWhitespace();
```

**行为**

按连续 ASCII 空白分词并忽略空项。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`List<String>`。按连续 ASCII 空白分词并忽略空项。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.splitWhitespace();
```

**源码**：[source/std/core.lep:707](../../source/std/core.lep#L707)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-49"></a>

### String::lines（50）

**声明**

```lepus
List<String> lines(bool keepEmpty = true);
```

**行为**

按 LF、CRLF 或 CR 分行，默认保留空行和末尾空项。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `keepEmpty` | `bool keepEmpty` | 是否保留分割产生的空项 | `true` |

**返回值**

`List<String>`。按 LF、CRLF 或 CR 分行，默认保留空行和末尾空项。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.lines(keepEmpty);
```

**源码**：[source/std/core.lep:710](../../source/std/core.lep#L710)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-50"></a>

### String::unicodeChars（51）

**声明**

```lepus
List<String> unicodeChars();
```

**行为**

严格按 UTF-8 Unicode 码点拆分，空串产生空列表。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`List<String>`。严格按 UTF-8 Unicode 码点拆分，空串产生空列表。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.unicodeChars();
```

**源码**：[source/std/core.lep:713](../../source/std/core.lep#L713)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-51"></a>

### String::bytes（52）

**声明**

```lepus
List<uint8> bytes();
```

**行为**

导出独立字节列表，修改列表不影响原字符串。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`List<uint8>`。导出独立字节列表，修改列表不影响原字符串。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.bytes();
```

**源码**：[source/std/core.lep:716](../../source/std/core.lep#L716)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-52"></a>

### String::join（53）

**声明**

```lepus
String join(List<String> items);
```

**行为**

用 separator 连接 parts。

以当前字符串为分隔符连接值列表，空列表返回空串。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `items` | `List<String> items` | 参与处理的列表元素 | 无 |

**返回值**

`String`。用 separator 连接 parts。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.join(items);
```

**源码**：[source/std/core.lep:719](../../source/std/core.lep#L719)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-53"></a>

### String::concat（54）

**声明**

```lepus
String concat(String text);
```

**行为**

返回两段文本的连接，不修改当前值。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `text` | `String text` | 输入文本，位置与长度通常按字节 | 无 |

**返回值**

`String`。返回两段文本的连接，不修改当前值。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.concat(text);
```

**源码**：[source/std/core.lep:722](../../source/std/core.lep#L722)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-54"></a>

### String::prepend（55）

**声明**

```lepus
String prepend(String text);
```

**行为**

返回在当前值前添加文本的新字符串。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `text` | `String text` | 输入文本，位置与长度通常按字节 | 无 |

**返回值**

`String`。返回在当前值前添加文本的新字符串。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.prepend(text);
```

**源码**：[source/std/core.lep:725](../../source/std/core.lep#L725)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-55"></a>

### Debug::debug（56）

**声明**

```lepus
String debug();
```

**行为**

返回供开发时检查的结构化文本；不作为持久化格式。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`String`。返回供开发时检查的结构化文本；不作为持久化格式。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.debug();
```

**源码**：[source/std/core.lep:731](../../source/std/core.lep#L731)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-56"></a>

### Error::toString（57）

**声明**

```lepus
String toString();
```

**行为**

由实现者将当前值转换为文本，Formattable 重载接收 style。

返回可直接显示给用户的错误说明。

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

**源码**：[source/std/core.lep:737](../../source/std/core.lep#L737)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-57"></a>

### MessageError::toString（58）

**声明**

```lepus
String toString();
```

**行为**

由实现者将当前值转换为文本，Formattable 重载接收 style。

返回原始诊断文本。

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

**源码**：[source/std/core.lep:745](../../source/std/core.lep#L745)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-58"></a>

### MessageError::from（59）

**声明**

```lepus
static MessageError from(String text);
```

**行为**

用首元素引用和长度创建非拥有 Span 视图。

从诊断文本构造拥有独立消息的错误。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `text` | `String text` | 输入文本，位置与长度通常按字节 | 无 |

**返回值**

`MessageError`。用首元素引用和长度创建非拥有 Span 视图。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
MessageError::from(text);
```

**源码**：[source/std/core.lep:747](../../source/std/core.lep#L747)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-59"></a>

### File::print（60）

**声明**

```lepus
int64 print(String text);
```

**行为**

写入文本，格式参数沿用 print 的规则。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `text` | `String text` | 待写入文本，支持由格式化调用生成。 | 无 |

**返回值**

`int64`。成功写入的字节数，失败沿用文件写入诊断。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.print(text);
```

**源码**：[source/std/core.lep:764](../../source/std/core.lep#L764)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-60"></a>

### File::println（61）

**声明**

```lepus
int64 println(String text = "");
```

**行为**

写入文本并追加一个 LF 换行。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `text` | `String text` | 待写入文本，省略时只写换行。 | `""` |

**返回值**

`int64`。包括换行的写入字节数。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.println(text);
```

**源码**：[source/std/core.lep:770](../../source/std/core.lep#L770)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-61"></a>

### readLine（62）

**声明**

```lepus
String readLine();
```

**行为**

读取一行标准输入，保留空格，去掉 LF/CRLF；EOF 返回空串，空行也返回空串。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`String`。读取的行文本；String 不允许内嵌零字节。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
readLine();
```

**源码**：[source/std/core.lep:839](../../source/std/core.lep#L839)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-62"></a>

### readLines（63）

**声明**

```lepus
String readLines(int32 N);
```

**行为**

最多读取 N 行，以 LF 连接，结果不额外追加换行；EOF 提前结束。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `N` | `int32 N` | 非负行数；零返回空串且不读取输入，负数触发 panic。 | 无 |

**返回值**

`String`。拼接后的文本，空行保留。

**错误与边界**

遵循本页“公共约定与错误边界”。
显式前置检查：`N>=0,"readLines requires a nonnegative line count"`。

**调用形式**

```lepus
readLines(N);
```

**源码**：[source/std/core.lep:843](../../source/std/core.lep#L843)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-63"></a>

### readLines（64）

**声明**

```lepus
String readLines(int64 N);
```

**行为**

接受默认整数实参的多行输入重载，检查后转为 int32 行数。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `N` | `int64 N` | 0 至 2147483647 的行数；超界触发 panic。 | 无 |

**返回值**

`String`。最多 N 行，以 LF 连接，EOF 提前结束。

**错误与边界**

遵循本页“公共约定与错误边界”。
显式前置检查：`N>=0 && N<=2147483647,"readLines requires a nonnegative int32 line count"`。

**调用形式**

```lepus
readLines(N);
```

**源码**：[source/std/core.lep:847](../../source/std/core.lep#L847)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-64"></a>

### toInt32（65）

**声明**

```lepus
Option<int32> toInt32(float64 value);
```

**行为**

向零截断为 int32；非有限数或截断后越界返回 None。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `float64 value` | 输入值或写入的新值 | 无 |

**返回值**

`Option<int32>`。向零截断为 int32；非有限数或截断后越界返回 None。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
toInt32(value);
```

**源码**：[source/std/core.lep:852](../../source/std/core.lep#L852)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-65"></a>

### toInt32（66）

**声明**

```lepus
Option<int32> toInt32(float32 value);
```

**行为**

向零截断为 int32；非有限数或截断后越界返回 None。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `float32 value` | 输入值或写入的新值 | 无 |

**返回值**

`Option<int32>`。向零截断为 int32；非有限数或截断后越界返回 None。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
toInt32(value);
```

**源码**：[source/std/core.lep:858](../../source/std/core.lep#L858)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-66"></a>

### toInt64（67）

**声明**

```lepus
int64 toInt64(float64 value);
```

**行为**

向零截断为 int64，非有限数或截断后越界时报错。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `float64 value` | 输入值或写入的新值 | 无 |

**返回值**

`int64`。向零截断为 int64，非有限数或截断后越界时报错。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
toInt64(value);
```

**源码**：[source/std/core.lep:860](../../source/std/core.lep#L860)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-67"></a>

### toInt64（68）

**声明**

```lepus
int64 toInt64(float32 value);
```

**行为**

向零截断为 int64，非有限数或截断后越界时报错。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `float32 value` | 输入值或写入的新值 | 无 |

**返回值**

`int64`。向零截断为 int64，非有限数或截断后越界时报错。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
toInt64(value);
```

**源码**：[source/std/core.lep:862](../../source/std/core.lep#L862)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-68"></a>

### float32ToBytes（69）

**声明**

```lepus
List<uint8> float32ToBytes(float32 value, bool bigEndian);
```

**行为**

按 IEEE 754 binary32 输出 4 个字节。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `float32 value` | 输入值或写入的新值 | 无 |
| `bigEndian` | `bool bigEndian` | true 为大端，false 为小端 | 无 |

**返回值**

`List<uint8>`。按 IEEE 754 binary32 输出 4 个字节。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
float32ToBytes(value, bigEndian);
```

**源码**：[source/std/core.lep:865](../../source/std/core.lep#L865)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-69"></a>

### float32ToBytes（70）

**声明**

```lepus
List<uint8> float32ToBytes(float32 value);
```

**行为**

默认小端编码。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `float32 value` | 输入值或写入的新值 | 无 |

**返回值**

`List<uint8>`。默认小端编码。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
float32ToBytes(value);
```

**源码**：[source/std/core.lep:867](../../source/std/core.lep#L867)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-70"></a>

### bytesToFloat32（71）

**声明**

```lepus
Option<float32> bytesToFloat32(List<uint8> bytes, bool bigEndian);
```

**行为**

读取列表前 4 字节，不足则返回 None。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `bytes` | `List<uint8> bytes` | 输入或输出字节存储 | 无 |
| `bigEndian` | `bool bigEndian` | true 为大端，false 为小端 | 无 |

**返回值**

`Option<float32>`。读取列表前 4 字节，不足则返回 None。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
bytesToFloat32(bytes, bigEndian);
```

**源码**：[source/std/core.lep:869](../../source/std/core.lep#L869)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-71"></a>

### bytesToFloat32（72）

**声明**

```lepus
Option<float32> bytesToFloat32(List<uint8> bytes);
```

**行为**

默认小端列表解码。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `bytes` | `List<uint8> bytes` | 输入或输出字节存储 | 无 |

**返回值**

`Option<float32>`。默认小端列表解码。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
bytesToFloat32(bytes);
```

**源码**：[source/std/core.lep:874](../../source/std/core.lep#L874)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-72"></a>

### bytesToFloat32（73）

**声明**

```lepus
Option<float32> bytesToFloat32(uint8 const& bytes, int64 count, bool bigEndian);
```

**行为**

从固定数组或指针读取前 4 字节；调用者保证 count 为有效长度。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `bytes` | `uint8 const& bytes` | 输入或输出字节存储 | 无 |
| `count` | `int64 count` | 元素或字节数量，具体单位见行为说明 | 无 |
| `bigEndian` | `bool bigEndian` | true 为大端，false 为小端 | 无 |

**返回值**

`Option<float32>`。从固定数组或指针读取前 4 字节；调用者保证 count 为有效长度。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
bytesToFloat32(bytes, count, bigEndian);
```

**源码**：[source/std/core.lep:876](../../source/std/core.lep#L876)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-73"></a>

### bytesToFloat32（74）

**声明**

```lepus
Option<float32> bytesToFloat32(uint8 const& bytes, int64 count);
```

**行为**

默认小端固定数组解码。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `bytes` | `uint8 const& bytes` | 输入或输出字节存储 | 无 |
| `count` | `int64 count` | 元素或字节数量，具体单位见行为说明 | 无 |

**返回值**

`Option<float32>`。默认小端固定数组解码。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
bytesToFloat32(bytes, count);
```

**源码**：[source/std/core.lep:881](../../source/std/core.lep#L881)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-74"></a>

### bytesToFloat32（75）

**声明**

```lepus
Option<float32> bytesToFloat32(uint8 bytes[], bool bigEndian);
```

**行为**

从运行时数组读取，自动获取长度。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `bytes` | `uint8 bytes[]` | 输入或输出字节存储 | 无 |
| `bigEndian` | `bool bigEndian` | true 为大端，false 为小端 | 无 |

**返回值**

`Option<float32>`。从运行时数组读取，自动获取长度。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
bytesToFloat32(bytes, bigEndian);
```

**源码**：[source/std/core.lep:883](../../source/std/core.lep#L883)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-75"></a>

### bytesToFloat32（76）

**声明**

```lepus
Option<float32> bytesToFloat32(uint8 bytes[]);
```

**行为**

默认小端运行时数组解码。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `bytes` | `uint8 bytes[]` | 输入或输出字节存储 | 无 |

**返回值**

`Option<float32>`。默认小端运行时数组解码。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
bytesToFloat32(bytes);
```

**源码**：[source/std/core.lep:885](../../source/std/core.lep#L885)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-76"></a>

### float32ToBytes（77）

**声明**

```lepus
bool float32ToBytes(float32 value, uint8 mut& bytes, int64 count, bool bigEndian);
```

**行为**

写入数组前 4 项；容量不足返回 false，数组保持不变。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `float32 value` | 输入值或写入的新值 | 无 |
| `bytes` | `uint8 mut& bytes` | 输入或输出字节存储 | 无 |
| `count` | `int64 count` | 元素或字节数量，具体单位见行为说明 | 无 |
| `bigEndian` | `bool bigEndian` | true 为大端，false 为小端 | 无 |

**返回值**

`bool`。写入数组前 4 项；容量不足返回 false，数组保持不变。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
float32ToBytes(value, bytes, count, bigEndian);
```

**源码**：[source/std/core.lep:887](../../source/std/core.lep#L887)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-77"></a>

### float32ToBytes（78）

**声明**

```lepus
bool float32ToBytes(float32 value, uint8 mut& bytes, int64 count);
```

**行为**

默认小端固定数组编码。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `float32 value` | 输入值或写入的新值 | 无 |
| `bytes` | `uint8 mut& bytes` | 输入或输出字节存储 | 无 |
| `count` | `int64 count` | 元素或字节数量，具体单位见行为说明 | 无 |

**返回值**

`bool`。默认小端固定数组编码。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
float32ToBytes(value, bytes, count);
```

**源码**：[source/std/core.lep:892](../../source/std/core.lep#L892)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-78"></a>

### float32ToBytes（79）

**声明**

```lepus
bool float32ToBytes(float32 value, uint8[] mut& bytes, bool bigEndian);
```

**行为**

编码到运行时数组的可变引用，自动获取容量；调用时传入 &array。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `float32 value` | 输入值或写入的新值 | 无 |
| `bytes` | `uint8[] mut& bytes` | 输入或输出字节存储 | 无 |
| `bigEndian` | `bool bigEndian` | true 为大端，false 为小端 | 无 |

**返回值**

`bool`。编码到运行时数组的可变引用，自动获取容量；调用时传入 &array。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
float32ToBytes(value, bytes, bigEndian);
```

**源码**：[source/std/core.lep:894](../../source/std/core.lep#L894)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-79"></a>

### float32ToBytes（80）

**声明**

```lepus
bool float32ToBytes(float32 value, uint8[] mut& bytes);
```

**行为**

默认小端运行时数组编码；调用时传入 &array。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `float32 value` | 输入值或写入的新值 | 无 |
| `bytes` | `uint8[] mut& bytes` | 输入或输出字节存储 | 无 |

**返回值**

`bool`。默认小端运行时数组编码；调用时传入 &array。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
float32ToBytes(value, bytes);
```

**源码**：[source/std/core.lep:899](../../source/std/core.lep#L899)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-80"></a>

### float64ToBytes（81）

**声明**

```lepus
List<uint8> float64ToBytes(float64 value, bool bigEndian);
```

**行为**

按 IEEE 754 binary64 输出 8 个字节。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `float64 value` | 输入值或写入的新值 | 无 |
| `bigEndian` | `bool bigEndian` | true 为大端，false 为小端 | 无 |

**返回值**

`List<uint8>`。按 IEEE 754 binary64 输出 8 个字节。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
float64ToBytes(value, bigEndian);
```

**源码**：[source/std/core.lep:902](../../source/std/core.lep#L902)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-81"></a>

### float64ToBytes（82）

**声明**

```lepus
List<uint8> float64ToBytes(float64 value);
```

**行为**

默认小端编码。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `float64 value` | 输入值或写入的新值 | 无 |

**返回值**

`List<uint8>`。默认小端编码。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
float64ToBytes(value);
```

**源码**：[source/std/core.lep:904](../../source/std/core.lep#L904)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-82"></a>

### bytesToFloat64（83）

**声明**

```lepus
Option<float64> bytesToFloat64(List<uint8> bytes, bool bigEndian);
```

**行为**

读取列表前 8 字节，不足则返回 None。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `bytes` | `List<uint8> bytes` | 输入或输出字节存储 | 无 |
| `bigEndian` | `bool bigEndian` | true 为大端，false 为小端 | 无 |

**返回值**

`Option<float64>`。读取列表前 8 字节，不足则返回 None。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
bytesToFloat64(bytes, bigEndian);
```

**源码**：[source/std/core.lep:906](../../source/std/core.lep#L906)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-83"></a>

### bytesToFloat64（84）

**声明**

```lepus
Option<float64> bytesToFloat64(List<uint8> bytes);
```

**行为**

默认小端列表解码。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `bytes` | `List<uint8> bytes` | 输入或输出字节存储 | 无 |

**返回值**

`Option<float64>`。默认小端列表解码。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
bytesToFloat64(bytes);
```

**源码**：[source/std/core.lep:911](../../source/std/core.lep#L911)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-84"></a>

### bytesToFloat64（85）

**声明**

```lepus
Option<float64> bytesToFloat64(uint8 const& bytes, int64 count, bool bigEndian);
```

**行为**

从固定数组或指针读取前 8 字节；调用者保证 count 为有效长度。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `bytes` | `uint8 const& bytes` | 输入或输出字节存储 | 无 |
| `count` | `int64 count` | 元素或字节数量，具体单位见行为说明 | 无 |
| `bigEndian` | `bool bigEndian` | true 为大端，false 为小端 | 无 |

**返回值**

`Option<float64>`。从固定数组或指针读取前 8 字节；调用者保证 count 为有效长度。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
bytesToFloat64(bytes, count, bigEndian);
```

**源码**：[source/std/core.lep:913](../../source/std/core.lep#L913)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-85"></a>

### bytesToFloat64（86）

**声明**

```lepus
Option<float64> bytesToFloat64(uint8 const& bytes, int64 count);
```

**行为**

默认小端固定数组解码。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `bytes` | `uint8 const& bytes` | 输入或输出字节存储 | 无 |
| `count` | `int64 count` | 元素或字节数量，具体单位见行为说明 | 无 |

**返回值**

`Option<float64>`。默认小端固定数组解码。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
bytesToFloat64(bytes, count);
```

**源码**：[source/std/core.lep:918](../../source/std/core.lep#L918)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-86"></a>

### bytesToFloat64（87）

**声明**

```lepus
Option<float64> bytesToFloat64(uint8 bytes[], bool bigEndian);
```

**行为**

从运行时数组读取，自动获取长度。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `bytes` | `uint8 bytes[]` | 输入或输出字节存储 | 无 |
| `bigEndian` | `bool bigEndian` | true 为大端，false 为小端 | 无 |

**返回值**

`Option<float64>`。从运行时数组读取，自动获取长度。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
bytesToFloat64(bytes, bigEndian);
```

**源码**：[source/std/core.lep:920](../../source/std/core.lep#L920)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-87"></a>

### bytesToFloat64（88）

**声明**

```lepus
Option<float64> bytesToFloat64(uint8 bytes[]);
```

**行为**

默认小端运行时数组解码。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `bytes` | `uint8 bytes[]` | 输入或输出字节存储 | 无 |

**返回值**

`Option<float64>`。默认小端运行时数组解码。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
bytesToFloat64(bytes);
```

**源码**：[source/std/core.lep:922](../../source/std/core.lep#L922)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-88"></a>

### float64ToBytes（89）

**声明**

```lepus
bool float64ToBytes(float64 value, uint8 mut& bytes, int64 count, bool bigEndian);
```

**行为**

写入数组前 8 项；容量不足返回 false，数组保持不变。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `float64 value` | 输入值或写入的新值 | 无 |
| `bytes` | `uint8 mut& bytes` | 输入或输出字节存储 | 无 |
| `count` | `int64 count` | 元素或字节数量，具体单位见行为说明 | 无 |
| `bigEndian` | `bool bigEndian` | true 为大端，false 为小端 | 无 |

**返回值**

`bool`。写入数组前 8 项；容量不足返回 false，数组保持不变。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
float64ToBytes(value, bytes, count, bigEndian);
```

**源码**：[source/std/core.lep:924](../../source/std/core.lep#L924)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-89"></a>

### float64ToBytes（90）

**声明**

```lepus
bool float64ToBytes(float64 value, uint8 mut& bytes, int64 count);
```

**行为**

默认小端固定数组编码。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `float64 value` | 输入值或写入的新值 | 无 |
| `bytes` | `uint8 mut& bytes` | 输入或输出字节存储 | 无 |
| `count` | `int64 count` | 元素或字节数量，具体单位见行为说明 | 无 |

**返回值**

`bool`。默认小端固定数组编码。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
float64ToBytes(value, bytes, count);
```

**源码**：[source/std/core.lep:929](../../source/std/core.lep#L929)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-90"></a>

### float64ToBytes（91）

**声明**

```lepus
bool float64ToBytes(float64 value, uint8[] mut& bytes, bool bigEndian);
```

**行为**

编码到运行时数组的可变引用，自动获取容量；调用时传入 &array。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `float64 value` | 输入值或写入的新值 | 无 |
| `bytes` | `uint8[] mut& bytes` | 输入或输出字节存储 | 无 |
| `bigEndian` | `bool bigEndian` | true 为大端，false 为小端 | 无 |

**返回值**

`bool`。编码到运行时数组的可变引用，自动获取容量；调用时传入 &array。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
float64ToBytes(value, bytes, bigEndian);
```

**源码**：[source/std/core.lep:931](../../source/std/core.lep#L931)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-91"></a>

### float64ToBytes（92）

**声明**

```lepus
bool float64ToBytes(float64 value, uint8[] mut& bytes);
```

**行为**

默认小端运行时数组编码；调用时传入 &array。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `float64 value` | 输入值或写入的新值 | 无 |
| `bytes` | `uint8[] mut& bytes` | 输入或输出字节存储 | 无 |

**返回值**

`bool`。默认小端运行时数组编码；调用时传入 &array。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
float64ToBytes(value, bytes);
```

**源码**：[source/std/core.lep:936](../../source/std/core.lep#L936)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-92"></a>

### assert（93）

**声明**

```lepus
void assert(bool condition);
```

**行为**

condition 为真时继续，为假时以指定诊断文本和可选行号触发 panic。

中止统一使用 std::core::panic，编译器与运行时通过 `__std_panic` 执行停机。 断言使用下面的标准库函数，它们在条件失败时调用 `std::core::panic`。 带消息的重载让标准库自己的前置检查 也能给出人话（例如 range 的步长）。 带 `int64 line` 的重载给文件末尾的系统宏 `$assert(...)` 用：宏把调用点的 `$line` 传进来，失败时就地报出**源码行号**，不必每处自己写文案。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `condition` | `bool condition` | 待验证的布尔条件 | 无 |

**返回值**

无返回值。

**错误与边界**

遵循本页“公共约定与错误边界”。
显式前置检查：`x) 的 x 为假"`。
实现包含 panic 分支；调用前应处理空值或失败状态。

**调用形式**

```lepus
assert(condition);
```

**源码**：[source/std/core.lep:949](../../source/std/core.lep#L949)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-93"></a>

### assert（94）

**声明**

```lepus
void assert(bool condition, String message);
```

**行为**

condition 为真时继续，为假时以指定诊断文本和可选行号触发 panic。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `condition` | `bool condition` | 待验证的布尔条件 | 无 |
| `message` | `String message` | 诊断文本或消息载荷 | 无 |

**返回值**

无返回值。

**错误与边界**

遵循本页“公共约定与错误边界”。
实现包含 panic 分支；调用前应处理空值或失败状态。

**调用形式**

```lepus
assert(condition, message);
```

**源码**：[source/std/core.lep:953](../../source/std/core.lep#L953)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-94"></a>

### assert（95）

**声明**

```lepus
void assert(bool condition, int64 line);
```

**行为**

condition 为真时继续，为假时以指定诊断文本和可选行号触发 panic。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `condition` | `bool condition` | 待验证的布尔条件 | 无 |
| `line` | `int64 line` | 用于诊断的源码行号 | 无 |

**返回值**

无返回值。

**错误与边界**

遵循本页“公共约定与错误边界”。
实现包含 panic 分支；调用前应处理空值或失败状态。

**调用形式**

```lepus
assert(condition, line);
```

**源码**：[source/std/core.lep:957](../../source/std/core.lep#L957)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-95"></a>

### assert（96）

**声明**

```lepus
void assert(bool condition, String message, int64 line);
```

**行为**

condition 为真时继续，为假时以指定诊断文本和可选行号触发 panic。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `condition` | `bool condition` | 待验证的布尔条件 | 无 |
| `message` | `String message` | 诊断文本或消息载荷 | 无 |
| `line` | `int64 line` | 用于诊断的源码行号 | 无 |

**返回值**

无返回值。

**错误与边界**

遵循本页“公共约定与错误边界”。
实现包含 panic 分支；调用前应处理空值或失败状态。

**调用形式**

```lepus
assert(condition, message, line);
```

**源码**：[source/std/core.lep:961](../../source/std/core.lep#L961)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-96"></a>

### Try::branch（97）

**声明**

```lepus
TryBranch<T, R> branch();
```

**行为**

只读拆分传播值，Continue 产生成功载荷，Break 携带失败载荷。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`TryBranch<T, R>`。只读拆分传播值，Continue 产生成功载荷，Break 携带失败载荷。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.branch();
```

**源码**：[source/std/core.lep:972](../../source/std/core.lep#L972)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-97"></a>

### TryMove::intoBranch（98）

**声明**

```lepus
TryBranch<T, R> intoBranch();
```

**行为**

在移动拥有值后拆分传播值，结果遵守成功与失败载荷的所有权规则。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`TryBranch<T, R>`。在移动拥有值后拆分传播值，结果遵守成功与失败载荷的所有权规则。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.intoBranch();
```

**源码**：[source/std/core.lep:974](../../source/std/core.lep#L974)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-98"></a>

### FromResidual::fromResidual（99）

**声明**

```lepus
static F fromResidual(R residual);
```

**行为**

静态地将匹配的传播载荷构造为当前函数的返回类型。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `residual` | `R residual` | 向当前函数返回类型传递的失败载荷。 | 无 |

**返回值**

`F`。静态地将匹配的传播载荷构造为当前函数的返回类型。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
FromResidual<F, R>::fromResidual(residual);
```

**源码**：[source/std/core.lep:976](../../source/std/core.lep#L976)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-99"></a>

### Option::branch（100）

**声明**

```lepus
TryBranch<T, NoneResidual> branch();
```

**行为**

只读拆分传播值，Continue 产生成功载荷，Break 携带失败载荷。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`TryBranch<T, NoneResidual>`。只读拆分传播值，Continue 产生成功载荷，Break 携带失败载荷。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.branch();
```

**源码**：[source/std/core.lep:981](../../source/std/core.lep#L981)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-100"></a>

### Option::intoBranch（101）

**声明**

```lepus
TryBranch<T, NoneResidual> intoBranch();
```

**行为**

在移动拥有值后拆分传播值，结果遵守成功与失败载荷的所有权规则。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`TryBranch<T, NoneResidual>`。在移动拥有值后拆分传播值，结果遵守成功与失败载荷的所有权规则。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.intoBranch();
```

**源码**：[source/std/core.lep:987](../../source/std/core.lep#L987)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-101"></a>

### Option::fromResidual（102）

**声明**

```lepus
static Option<T> fromResidual(NoneResidual residual);
```

**行为**

静态地将匹配的传播载荷构造为当前函数的返回类型。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `residual` | `NoneResidual residual` | 向当前函数返回类型传递的失败载荷。 | 无 |

**返回值**

`Option<T>`。静态地将匹配的传播载荷构造为当前函数的返回类型。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
Option<T>::fromResidual(residual);
```

**源码**：[source/std/core.lep:991](../../source/std/core.lep#L991)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-102"></a>

### Option::isValue（103）

**声明**

```lepus
bool isValue();
```

**行为**

判断 Option 是否含 Value。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`bool`。判断 Option 是否含 Value。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.isValue();
```

**源码**：[source/std/core.lep:992](../../source/std/core.lep#L992)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-103"></a>

### Option::isNone（104）

**声明**

```lepus
bool isNone();
```

**行为**

判断 Option 是否为 None。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`bool`。判断 Option 是否为 None。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.isNone();
```

**源码**：[source/std/core.lep:1001](../../source/std/core.lep#L1001)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-104"></a>

### Option::take（105）

**声明**

```lepus
T take();
```

**行为**

转移整个结果并取出成功载荷；源结果随后不可使用。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`T`。转移整个结果并取出成功载荷；源结果随后不可使用。

**错误与边界**

遵循本页“公共约定与错误边界”。
实现包含 panic 分支；调用前应处理空值或失败状态。

**调用形式**

```lepus
obj.take();
```

**源码**：[source/std/core.lep:1004](../../source/std/core.lep#L1004)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-105"></a>

### Option::unwrap（106）

**声明**

```lepus
T unwrap();
```

**行为**

提取成功载荷；None/Fail 时 panic。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`T`。提取成功载荷；None/Fail 时 panic。

**错误与边界**

遵循本页“公共约定与错误边界”。
实现包含 panic 分支；调用前应处理空值或失败状态。

**调用形式**

```lepus
obj.unwrap();
```

**源码**：[source/std/core.lep:1014](../../source/std/core.lep#L1014)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-106"></a>

### Option::unwrapOr（107）

**声明**

```lepus
T unwrapOr(T fallback);
```

**行为**

提取成功载荷；None/Fail 时返回 fallback。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `fallback` | `T fallback` | 失败/空值分支的替代值 | 无 |

**返回值**

`T`。提取成功载荷；None/Fail 时返回 fallback。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.unwrapOr(fallback);
```

**源码**：[source/std/core.lep:1023](../../source/std/core.lep#L1023)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-107"></a>

### Option::expect（108）

**声明**

```lepus
T expect(String message);
```

**行为**

提取成功载荷；None/Fail 时以 message 触发 panic。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `message` | `String message` | 诊断文本或消息载荷 | 无 |

**返回值**

`T`。提取成功载荷；None/Fail 时以 message 触发 panic。

**错误与边界**

遵循本页“公共约定与错误边界”。
实现包含 panic 分支；调用前应处理空值或失败状态。

**调用形式**

```lepus
obj.expect(message);
```

**源码**：[source/std/core.lep:1032](../../source/std/core.lep#L1032)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-108"></a>

### Option::transform（109）

**声明**

```lepus
Option<T> transform(T fn(T) mapper);
```

**行为**

成功时调用 mapper 转换载荷，失败分支保持不变。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `mapper` | `T fn(T) mapper` | 转换回调，签名决定输入和输出类型 | 无 |

**返回值**

`Option<T>`。成功时调用 mapper 转换载荷，失败分支保持不变。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.transform(mapper);
```

**源码**：[source/std/core.lep:1041](../../source/std/core.lep#L1041)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-109"></a>

### Option::andThen（110）

**声明**

```lepus
Option<T> andThen(Option<T> fn(T) mapper);
```

**行为**

成功时调用返回同族 Option/Result 的 mapper，将结果展平。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `mapper` | `Option<T> fn(T) mapper` | 转换回调，签名决定输入和输出类型 | 无 |

**返回值**

`Option<T>`。成功时调用返回同族 Option/Result 的 mapper，将结果展平。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.andThen(mapper);
```

**源码**：[source/std/core.lep:1050](../../source/std/core.lep#L1050)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-110"></a>

### Result::branch（111）

**声明**

```lepus
TryBranch<T, ErrorResidual<E>> branch();
```

**行为**

只读拆分传播值，Continue 产生成功载荷，Break 携带失败载荷。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`TryBranch<T, ErrorResidual<E>>`。只读拆分传播值，Continue 产生成功载荷，Break 携带失败载荷。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.branch();
```

**源码**：[source/std/core.lep:1071](../../source/std/core.lep#L1071)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-111"></a>

### Result::intoBranch（112）

**声明**

```lepus
TryBranch<T, ErrorResidual<E>> intoBranch();
```

**行为**

在移动拥有值后拆分传播值，结果遵守成功与失败载荷的所有权规则。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`TryBranch<T, ErrorResidual<E>>`。在移动拥有值后拆分传播值，结果遵守成功与失败载荷的所有权规则。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.intoBranch();
```

**源码**：[source/std/core.lep:1081](../../source/std/core.lep#L1081)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-112"></a>

### Result::fromResidual（113）

**声明**

```lepus
static Result<T,E> fromResidual(ErrorResidual<E> residual);
```

**行为**

静态地将匹配的传播载荷构造为当前函数的返回类型。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `residual` | `ErrorResidual<E> residual` | 向当前函数返回类型传递的失败载荷。 | 无 |

**返回值**

`Result<T,E>`。静态地将匹配的传播载荷构造为当前函数的返回类型。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
Result<T, E>::fromResidual(residual);
```

**源码**：[source/std/core.lep:1087](../../source/std/core.lep#L1087)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-113"></a>

### Result::ok（114）

**声明**

```lepus
Option<T> ok();
```

**行为**

成功时保留值，失败时明确丢弃错误信息并返回 None。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`Option<T>`。成功时保留值，失败时明确丢弃错误信息并返回 None。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.ok();
```

**源码**：[source/std/core.lep:1088](../../source/std/core.lep#L1088)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-114"></a>

### Result::isOk（115）

**声明**

```lepus
bool isOk();
```

**行为**

判断 Result 是否为 Ok。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`bool`。判断 Result 是否为 Ok。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.isOk();
```

**源码**：[source/std/core.lep:1094](../../source/std/core.lep#L1094)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-115"></a>

### Result::isErr（116）

**声明**

```lepus
bool isErr();
```

**行为**

判断 Result 是否为 Fail。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`bool`。判断 Result 是否为 Fail。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.isErr();
```

**源码**：[source/std/core.lep:1103](../../source/std/core.lep#L1103)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-116"></a>

### Result::take（117）

**声明**

```lepus
T take();
```

**行为**

转移整个结果并取出成功载荷；源结果随后不可使用。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`T`。转移整个结果并取出成功载荷；源结果随后不可使用。

**错误与边界**

遵循本页“公共约定与错误边界”。
实现包含 panic 分支；调用前应处理空值或失败状态。

**调用形式**

```lepus
obj.take();
```

**源码**：[source/std/core.lep:1106](../../source/std/core.lep#L1106)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-117"></a>

### Result::unwrap（118）

**声明**

```lepus
T unwrap();
```

**行为**

提取成功载荷；None/Fail 时 panic。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`T`。提取成功载荷；None/Fail 时 panic。

**错误与边界**

遵循本页“公共约定与错误边界”。
实现包含 panic 分支；调用前应处理空值或失败状态。

**调用形式**

```lepus
obj.unwrap();
```

**源码**：[source/std/core.lep:1116](../../source/std/core.lep#L1116)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-118"></a>

### Result::unwrapOr（119）

**声明**

```lepus
T unwrapOr(T fallback);
```

**行为**

提取成功载荷；None/Fail 时返回 fallback。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `fallback` | `T fallback` | 失败/空值分支的替代值 | 无 |

**返回值**

`T`。提取成功载荷；None/Fail 时返回 fallback。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.unwrapOr(fallback);
```

**源码**：[source/std/core.lep:1125](../../source/std/core.lep#L1125)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-119"></a>

### Result::expect（120）

**声明**

```lepus
T expect(String message);
```

**行为**

提取成功载荷；None/Fail 时以 message 触发 panic。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `message` | `String message` | 诊断文本或消息载荷 | 无 |

**返回值**

`T`。提取成功载荷；None/Fail 时以 message 触发 panic。

**错误与边界**

遵循本页“公共约定与错误边界”。
实现包含 panic 分支；调用前应处理空值或失败状态。

**调用形式**

```lepus
obj.expect(message);
```

**源码**：[source/std/core.lep:1134](../../source/std/core.lep#L1134)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-120"></a>

### Result::takeError（121）

**声明**

```lepus
E takeError();
```

**行为**

转移整个结果并取出错误载荷；源结果随后不可使用。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`E`。转移整个结果并取出错误载荷；源结果随后不可使用。

**错误与边界**

遵循本页“公共约定与错误边界”。
实现包含 panic 分支；调用前应处理空值或失败状态。

**调用形式**

```lepus
obj.takeError();
```

**源码**：[source/std/core.lep:1144](../../source/std/core.lep#L1144)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-121"></a>

### Result::error（122）

**声明**

```lepus
E error();
```

**行为**

提取 Fail 载荷；Ok 时 panic。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`E`。提取 Fail 载荷；Ok 时 panic。

**错误与边界**

遵循本页“公共约定与错误边界”。
实现包含 panic 分支；调用前应处理空值或失败状态。

**调用形式**

```lepus
obj.error();
```

**源码**：[source/std/core.lep:1154](../../source/std/core.lep#L1154)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-122"></a>

### Result::transform（123）

**声明**

```lepus
Result<T, E> transform(T fn(T) mapper);
```

**行为**

成功时调用 mapper 转换载荷，失败分支保持不变。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `mapper` | `T fn(T) mapper` | 转换回调，签名决定输入和输出类型 | 无 |

**返回值**

`Result<T, E>`。成功时调用 mapper 转换载荷，失败分支保持不变。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.transform(mapper);
```

**源码**：[source/std/core.lep:1163](../../source/std/core.lep#L1163)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-123"></a>

### Result::andThen（124）

**声明**

```lepus
Result<T, E> andThen(Result<T, E> fn(T) mapper);
```

**行为**

成功时调用返回同族 Option/Result 的 mapper，将结果展平。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `mapper` | `Result<T, E> fn(T) mapper` | 转换回调，签名决定输入和输出类型 | 无 |

**返回值**

`Result<T, E>`。成功时调用返回同族 Option/Result 的 mapper，将结果展平。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.andThen(mapper);
```

**源码**：[source/std/core.lep:1172](../../source/std/core.lep#L1172)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-124"></a>

### NextValue::nextValue（125）

**声明**

```lepus
Option<T> nextValue(T end, bool inclusive);
```

**行为**

使用接收者的游标状态、end 与 inclusive 产生下一项，并推进游标；返回 None 结束区间。

取得下一项，由实现判断边界并维护游标状态。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `end` | `T end` | 区间终点。 | 无 |
| `inclusive` | `bool inclusive` | 是否包含终点。 | 无 |

**返回值**

`Option<T>`。下一项，或表示遍历结束的 None。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.nextValue(end, inclusive);
```

**源码**：[source/std/core.lep:1196](../../source/std/core.lep#L1196)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-125"></a>

### Pair::create（126）

**声明**

```lepus
static Pair<K, V> create(K first, V second);
```

**行为**

创建Pair，返回可供后续操作的值。

从两个值复制构造键值对。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `first` | `K first` | 键或第一项。 | 无 |
| `second` | `V second` | 值或第二项。 | 无 |

**返回值**

`Pair<K, V>`。拥有独立字段值的 Pair。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
Pair<K, V>::create(first, second);
```

**源码**：[source/std/core.lep:1252](../../source/std/core.lep#L1252)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-126"></a>

### Pair::create（127）

**声明**

```lepus
static Pair<K, V> create((K, V) value);
```

**行为**

创建Pair，返回可供后续操作的值。

从二元组复制构造键值对。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `(K, V) value` | 提供 first 与 second 的两个值。 | 无 |

**返回值**

`Pair<K, V>`。拥有独立字段值的 Pair。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
Pair<K, V>::create(value);
```

**源码**：[source/std/core.lep:1262](../../source/std/core.lep#L1262)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-127"></a>

### Pair::toTuple（128）

**声明**

```lepus
(K, V) toTuple();
```

**行为**

将键值对的两个字段复制为二元组。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`(K, V)`。按 first、second 顺序保存独立值的二元组。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.toTuple();
```

**源码**：[source/std/core.lep:1271](../../source/std/core.lep#L1271)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-128"></a>

### isType（129）

**声明**

```lepus
bool isType<T>(Any const& value);
```

**行为**

判断盒子是否保存精确的 T 类型，不比较公开整数编号。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `Any const& value` | 输入值或写入的新值 | 无 |

**返回值**

`bool`。判断盒子是否保存精确的 T 类型，不比较公开整数编号。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
isType<T>(value);
```

**源码**：[source/std/core.lep:1294](../../source/std/core.lep#L1294)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-129"></a>

### tryGet（130）

**声明**

```lepus
Option<T> tryGet<T>(Any const& value);
```

**行为**

类型匹配时复制拥有值；不匹配或空盒子返回 None。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `Any const& value` | 输入值或写入的新值 | 无 |

**返回值**

`Option<T>`。类型匹配时复制拥有值；不匹配或空盒子返回 None。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
tryGet<T>(value);
```

**源码**：[source/std/core.lep:1296](../../source/std/core.lep#L1296)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-130"></a>

### tryGetReference（131）

**声明**

```lepus
Option<T const&> tryGetReference<T>(Any const& value);
```

**行为**

安全借用只读值或兼容接口，引用不得超过盒子的寿命。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `Any const& value` | 输入值或写入的新值 | 无 |

**返回值**

`Option<T const&>`。安全借用只读值或兼容接口，引用不得超过盒子的寿命。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
tryGetReference<T>(value);
```

**源码**：[source/std/core.lep:1302](../../source/std/core.lep#L1302)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-131"></a>

### tryGetMut（132）

**声明**

```lepus
Option<T mut&> tryGetMut<T>(Any mut& value);
```

**行为**

安全借用可写值或兼容接口，要求盒子可写。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `Any mut& value` | 输入值或写入的新值 | 无 |

**返回值**

`Option<T mut&>`。安全借用可写值或兼容接口，要求盒子可写。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
tryGetMut<T>(value);
```

**源码**：[source/std/core.lep:1307](../../source/std/core.lep#L1307)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-132"></a>

### stringBytes（133）

**声明**

```lepus
List<uint8> stringBytes(String text);
```

**行为**

返回字符串的完整 UTF-8 字节副本，不执行字符边界索引。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `text` | `String text` | 输入文本，位置与长度通常按字节 | 无 |

**返回值**

`List<uint8>`。返回字符串的完整 UTF-8 字节副本，不执行字符边界索引。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
stringBytes(text);
```

**源码**：[source/std/core.lep:1324](../../source/std/core.lep#L1324)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-133"></a>

### BarePtr::leftAdd（134）

**声明**

```lepus
BarePtr<T> leftAdd(int64 elements);
```

**行为**

实现 p + elements；按静态元素大小偏移，不访问内存或检测运行时类型。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `elements` | `int64 elements` | 按静态元素大小计算的指针偏移数量。 | 无 |

**返回值**

`BarePtr<T>`。实现 p + elements；按静态元素大小偏移，不访问内存或检测运行时类型。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.leftAdd(elements);
```

**源码**：[source/std/core.lep:1377](../../source/std/core.lep#L1377)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-134"></a>

### BarePtr::rightAdd（135）

**声明**

```lepus
BarePtr<T> rightAdd(int64 elements);
```

**行为**

实现 elements + p，与 p + elements 相同。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `elements` | `int64 elements` | 按静态元素大小计算的指针偏移数量。 | 无 |

**返回值**

`BarePtr<T>`。实现 elements + p，与 p + elements 相同。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.rightAdd(elements);
```

**源码**：[source/std/core.lep:1384](../../source/std/core.lep#L1384)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-135"></a>

### BarePtr::leftSub（136）

**声明**

```lepus
BarePtr<T> leftSub(int64 elements);
```

**行为**

实现 p - elements；按静态元素大小反向偏移。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `elements` | `int64 elements` | 按静态元素大小计算的指针偏移数量。 | 无 |

**返回值**

`BarePtr<T>`。实现 p - elements；按静态元素大小反向偏移。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.leftSub(elements);
```

**源码**：[source/std/core.lep:1386](../../source/std/core.lep#L1386)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-136"></a>

### BarePtr::leftSub（137）

**声明**

```lepus
int64 leftSub(BarePtr<T> other);
```

**行为**

实现 p - other，验证两者属于同一存活分配后返回完整元素距离。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `other` | `BarePtr<T> other` | 另一操作数 | 无 |

**返回值**

`int64`。实现 p - other，验证两者属于同一存活分配后返回完整元素距离。

**错误与边界**

遵循本页“公共约定与错误边界”。
显式前置检查：`self.storage != 0u && self.storage == other.storage, "裸指针相减要求同一来源分配"`。
显式前置检查：`bytes % __rawSize(T) == 0`。

**调用形式**

```lepus
obj.leftSub(other);
```

**源码**：[source/std/core.lep:1393](../../source/std/core.lep#L1393)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-137"></a>

### barePointer（138）

**声明**

```lepus
unsafe BarePtr<T> barePointer<T>(RawPtr<T> pointer);
```

**行为**

取得原始分配中的裸地址，允许未初始化及尾后位置，不检测元素类型。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `pointer` | `RawPtr<T> pointer` | 有元素类型的原始指针；调用方保证其分配与元素寿命有效。 | 无 |

**返回值**

`BarePtr<T>`。取得原始分配中的裸地址，允许未初始化及尾后位置，不检测元素类型。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
barePointer(pointer);
```

**源码**：[source/std/core.lep:1440](../../source/std/core.lep#L1440)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-138"></a>

### pointerFromAddress（139）

**声明**

```lepus
unsafe BarePtr<T> pointerFromAddress<T>(int64 address);
```

**行为**

将数值字节地址解释为裸指针，不验证类型、初始化状态或可解引用性。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `address` | `int64 address` | Lepus 逻辑字节地址；调用方保证布局与有效访问范围。 | 无 |

**返回值**

`BarePtr<T>`。将数值字节地址解释为裸指针，不验证类型、初始化状态或可解引用性。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
pointerFromAddress<T>(address);
```

**源码**：[source/std/core.lep:1450](../../source/std/core.lep#L1450)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-139"></a>

### dereference（140）

**声明**

```lepus
unsafe T mut& dereference<T>(BarePtr<T> pointer);
```

**行为**

将裸地址解释为可变引用；调用者负责布局、对齐、初始化和所有权正确性。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `pointer` | `BarePtr<T> pointer` | 有元素类型的原始指针；调用方保证其分配与元素寿命有效。 | 无 |

**返回值**

`T mut&`。将裸地址解释为可变引用；调用者负责布局、对齐、初始化和所有权正确性。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
dereference(pointer);
```

**源码**：[source/std/core.lep:1470](../../source/std/core.lep#L1470)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-140"></a>

### memcpy（141）

**声明**

```lepus
unsafe void memcpy<T,U>(BarePtr<T> destination, BarePtr<U> source, int64 bytes);
```

**行为**

按字节复制完整范围；不检测类型或处理资源所有权，非零范围必须有效且不重叠。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `destination` | `BarePtr<T> destination` | 目标逻辑地址。 | 无 |
| `source` | `BarePtr<U> source` | 源逻辑地址；可与目标使用不同静态元素类型。 | 无 |
| `bytes` | `int64 bytes` | 复制的字节数，必须非负；零长度允许空指针。 | 无 |

**返回值**

无返回值。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
memcpy(destination, source, bytes);
```

**源码**：[source/std/core.lep:1481](../../source/std/core.lep#L1481)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-141"></a>

### memset（142）

**声明**

```lepus
unsafe void memset<T>(BarePtr<T> destination, uint8 value, int64 bytes);
```

**行为**

按字节填充完整范围；不检测类型或清理旧值，调用者负责资源与初始化状态。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `destination` | `BarePtr<T> destination` | 目标逻辑地址。 | 无 |
| `value` | `uint8 value` | 重复写入的单个字节。 | 无 |
| `bytes` | `int64 bytes` | 填充的字节数，必须非负；零长度允许空指针。 | 无 |

**返回值**

无返回值。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
memset(destination, value, bytes);
```

**源码**：[source/std/core.lep:1494](../../source/std/core.lep#L1494)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-142"></a>

### initAt（143）

**声明**

```lepus
unsafe void initAt<T>(RawPtr<T> pointer, T value);
```

**行为**

在原始槽位就地接管一个完整值，目标必须未初始化。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `pointer` | `RawPtr<T> pointer` | 有元素类型的原始指针；调用方保证其分配与元素寿命有效。 | 无 |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

无返回值。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
initAt(pointer, value);
```

**源码**：[source/std/core.lep:1502](../../source/std/core.lep#L1502)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-143"></a>

### readAt（144）

**声明**

```lepus
unsafe T readAt<T>(RawPtr<T> pointer);
```

**行为**

从已初始化槽位取得独立副本；需要调用方保证指针与存储寿命有效。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `pointer` | `RawPtr<T> pointer` | 有元素类型的原始指针；调用方保证其分配与元素寿命有效。 | 无 |

**返回值**

`T`。从已初始化槽位取得独立副本；需要调用方保证指针与存储寿命有效。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
readAt(pointer);
```

**源码**：[source/std/core.lep:1512](../../source/std/core.lep#L1512)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-144"></a>

### borrowAt（145）

**声明**

```lepus
unsafe T mut& borrowAt<T>(RawPtr<T> pointer);
```

**行为**

借出已初始化元素；调用方必须保证引用不超过原始存储的有效寿命。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `pointer` | `RawPtr<T> pointer` | 有元素类型的原始指针；调用方保证其分配与元素寿命有效。 | 无 |

**返回值**

`T mut&`。借出已初始化元素；调用方必须保证引用不超过原始存储的有效寿命。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
borrowAt(pointer);
```

**源码**：[source/std/core.lep:1521](../../source/std/core.lep#L1521)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-145"></a>

### withBorrowAt（146）

**声明**

```lepus
unsafe void withBorrowAt<T>(RawPtr<T> pointer, void fn(T mut&) use);
```

**行为**

在一次同步回调内可写地借用槽位。回调结束时释放借用，期间禁止移出、搬移或关闭存储。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `pointer` | `RawPtr<T> pointer` | 要借用的已初始化槽位。 | 无 |
| `use` | `void fn(T mut&) use` | 持有元素可写引用的同步回调。 | 无 |

**返回值**

无返回值。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
withBorrowAt<T>(pointer, use);
```

**源码**：[source/std/core.lep:1531](../../source/std/core.lep#L1531)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-146"></a>

### takeAt（147）

**声明**

```lepus
unsafe T takeAt<T>(RawPtr<T> pointer);
```

**行为**

移出已初始化槽位；成功后原槽位不再持有元素。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `pointer` | `RawPtr<T> pointer` | 有元素类型的原始指针；调用方保证其分配与元素寿命有效。 | 无 |

**返回值**

`T`。移出已初始化槽位；成功后原槽位不再持有元素。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
takeAt(pointer);
```

**源码**：[source/std/core.lep:1541](../../source/std/core.lep#L1541)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-147"></a>

### dropAt（148）

**声明**

```lepus
unsafe void dropAt<T>(RawPtr<T> pointer);
```

**行为**

清理恰好一个已初始化元素，保留原始容量。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `pointer` | `RawPtr<T> pointer` | 有元素类型的原始指针；调用方保证其分配与元素寿命有效。 | 无 |

**返回值**

无返回值。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
dropAt(pointer);
```

**源码**：[source/std/core.lep:1552](../../source/std/core.lep#L1552)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-148"></a>

### relocateNonOverlapping（149）

**声明**

```lepus
unsafe void relocateNonOverlapping<T>(RawPtr<T> destination, RawPtr<T> source, int64 count);
```

**行为**

验证移动范围后逐元素转移所有权，目标范围必须全部未初始化且不重叠。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `destination` | `RawPtr<T> destination` | 可写目标存储首地址 | 无 |
| `source` | `RawPtr<T> source` | 输入游标、存储或事件来源值 | 无 |
| `count` | `int64 count` | 元素或字节数量，具体单位见行为说明 | 无 |

**返回值**

无返回值。

**错误与边界**

遵循本页“公共约定与错误边界”。
显式前置检查：`count >= 0`。
显式前置检查：`source.index >= 0 && source.index <= sourceCapacity && count <= sourceCapacity - source.index`。
显式前置检查：`destination.index >= 0 && destination.index <= destinationCapacity && count <= destinationCapacity - destination.index`。
显式前置检查：`source.storage != destination.storage || destination.index >= source.index + count || source.index >= destination.index + count`。

**调用形式**

```lepus
relocateNonOverlapping(destination, source, count);
```

**源码**：[source/std/core.lep:1559](../../source/std/core.lep#L1559)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-149"></a>

### RawStorage::allocate（150）

**声明**

```lepus
static RawStorage<T> allocate(int64 capacity);
```

**行为**



**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `capacity` | `int64 capacity` | 原始存储的元素容量，必须非负。 | 无 |

**返回值**

`RawStorage<T>`。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
RawStorage<T>::allocate(capacity);
```

**源码**：[source/std/core.lep:1593](../../source/std/core.lep#L1593)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-150"></a>

### RawStorage::capacity（151）

**声明**

```lepus
int64 capacity();
```

**行为**



**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`int64`。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.capacity();
```

**源码**：[source/std/core.lep:1601](../../source/std/core.lep#L1601)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-151"></a>

### RawStorage::pointer（152）

**声明**

```lepus
RawPtr<T> pointer(int64 index);
```

**行为**



**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `index` | `int64 index` | 从 0 开始的位置 | 无 |

**返回值**

`RawPtr<T>`。

**错误与边界**

遵循本页“公共约定与错误边界”。
显式前置检查：`self.handle != 0u && index >= 0 && index <= self.capacity()`。

**调用形式**

```lepus
obj.pointer(index);
```

**源码**：[source/std/core.lep:1606](../../source/std/core.lep#L1606)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-152"></a>

### RawStorage::isInitialized（153）

**声明**

```lepus
bool isInitialized(int64 index);
```

**行为**



**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `index` | `int64 index` | 从 0 开始的位置 | 无 |

**返回值**

`bool`。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.isInitialized(index);
```

**源码**：[source/std/core.lep:1614](../../source/std/core.lep#L1614)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-153"></a>

### RawStorage::close（154）

**声明**

```lepus
void close();
```

**行为**



**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

无返回值。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.close();
```

**源码**：[source/std/core.lep:1618](../../source/std/core.lep#L1618)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-154"></a>

### Box::newBox（155）

**声明**

```lepus
static Box<T> newBox(T value);
```

**行为**

把一个值放入独占的堆盒。可复制的实参可按值传入，转移所有权时使用 move。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 要装箱的值。 | 无 |

**返回值**

`Box<T>`。持有该值的 Box。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
Box<T>::newBox(value);
```

**源码**：[source/std/core.lep:1645](../../source/std/core.lep#L1645)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-155"></a>

### Box::isEmpty（156）

**声明**

```lepus
bool isEmpty();
```

**行为**

查询当前 Box 是否为空。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`bool`。默认构造或关闭后为 true。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.isEmpty();
```

**源码**：[source/std/core.lep:1655](../../source/std/core.lep#L1655)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-156"></a>

### Box::get（157）

**声明**

```lepus
T get();
```

**行为**

复制盒中的值，要求 T 可复制。空盒触发断言。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`T`。盒中值的独立副本。

**错误与边界**

遵循本页“公共约定与错误边界”。
显式前置检查：`!self.isEmpty(), "Box 为空"`。

**调用形式**

```lepus
obj.get();
```

**源码**：[source/std/core.lep:1660](../../source/std/core.lep#L1660)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-157"></a>

### Box::take（158）

**声明**

```lepus
T take();
```

**行为**

消耗当前 Box 并释放堆存储。空盒触发断言。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`T`。从盒中移出的值。

**错误与边界**

遵循本页“公共约定与错误边界”。
显式前置检查：`!self.isEmpty(), "Box 为空"`。

**调用形式**

```lepus
obj.take();
```

**源码**：[source/std/core.lep:1668](../../source/std/core.lep#L1668)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-158"></a>

### Box::close（159）

**声明**

```lepus
void close();
```

**行为**

销毁仍在盒中的值并释放堆存储。可以重复关闭。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

无返回值。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.close();
```

**源码**：[source/std/core.lep:1675](../../source/std/core.lep#L1675)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-159"></a>

### range（160）

**声明**

```lepus
__Range range(int64 start, int64 stop);
```

**行为**

创建半开整数区间；正步长小于 stop，负步长大于 stop，步长不能为零。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `start` | `int64 start` | 起始值 | 无 |
| `stop` | `int64 stop` | 不包含的终止值 | 无 |

**返回值**

`__Range`。创建半开整数区间；正步长小于 stop，负步长大于 stop，步长不能为零。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
range(start, stop);
```

**源码**：[source/lepus_generic.cpp:2283](../../source/lepus_generic.cpp#L2283)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-160"></a>

### range（161）

**声明**

```lepus
__Range range(int64 start, int64 stop, int64 step);
```

**行为**

创建半开整数区间；正步长小于 stop，负步长大于 stop，步长不能为零。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `start` | `int64 start` | 起始值 | 无 |
| `stop` | `int64 stop` | 不包含的终止值 | 无 |
| `step` | `int64 step` | 步长或时间增量 | 无 |

**返回值**

`__Range`。创建半开整数区间；正步长小于 stop，负步长大于 stop，步长不能为零。

**错误与边界**

遵循本页“公共约定与错误边界”。
显式前置检查：`step != 0, "range 的步长不能为 0"`。

**调用形式**

```lepus
range(start, stop, step);
```

**源码**：[source/lepus_generic.cpp:2291](../../source/lepus_generic.cpp#L2291)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-161"></a>

### rangeEq（162）

**声明**

```lepus
__Interval_int64 rangeEq(int64 start, int64 stop);
```

**行为**

构造包含起点和终点的值区间；整数和 char 直接计算，其他类型通过 NextValue<T> 迭代。match 区间模式仅支持整数和 char。

构造包含起点和终点的整数区间。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `start` | `int64 start` | 区间起点。 | 无 |
| `stop` | `int64 stop` | 包含在区间内的终点。 | 无 |

**返回值**

`__Interval_int64`。保存独立游标的闭区间值。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
rangeEq(start, stop);
```

**源码**：[source/lepus_generic.cpp:2306](../../source/lepus_generic.cpp#L2306)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-162"></a>

### Drop::drop（163）

**声明**

```lepus
void drop();
```

**行为**

释放对象拥有的资源，调用后遵守本模块失效规则。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

无返回值。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.drop();
```

**源码**：[source/lepus_generic.cpp:2512](../../source/lepus_generic.cpp#L2512)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-163"></a>

### Closeable::close（164）

**声明**

```lepus
void close();
```

**行为**

关闭底层资源；文档节点会使同文档所有视图失效。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

无返回值。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.close();
```

**源码**：[source/lepus_generic.cpp:2520](../../source/lepus_generic.cpp#L2520)。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-164"></a>

### print（165）

**声明**

```lepus
int64 print(String format, Any args...);
```

**行为**

输出格式化文本，返回过程状态；占位符和变长实参由编译器检查。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `format` | `String format` | 格式模板，使用 {} 占位符 | 无 |
| `args` | `Any args...` | 实参列表 | 无 |

**返回值**

`int64`。输出格式化文本，返回过程状态；占位符和变长实参由编译器检查。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
print(format, args);
```

**源码**：[source/semantics/boxing.cpp:1357](../../source/semantics/boxing.cpp#L1357)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-165"></a>

### println（166）

**声明**

```lepus
int64 println(String format, Any args...);
```

**行为**

输出格式化文本并换行。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `format` | `String format` | 格式模板，使用 {} 占位符 | 无 |
| `args` | `Any args...` | 实参列表 | 无 |

**返回值**

`int64`。输出格式化文本并换行。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
println(format, args);
```

**源码**：[source/semantics/boxing.cpp:1357](../../source/semantics/boxing.cpp#L1357)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-166"></a>

### eprint（167）

**声明**

```lepus
int64 eprint(String format, Any args...);
```

**行为**

格式化写入标准错误流，不追加换行。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `format` | `String format` | 格式模板，使用 {} 占位符 | 无 |
| `args` | `Any args...` | 实参列表 | 无 |

**返回值**

`int64`。格式化写入标准错误流，不追加换行。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
eprint(format, args);
```

**源码**：[source/semantics/formatted_output.cpp:15](../../source/semantics/formatted_output.cpp#L15)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-167"></a>

### eprintln（168）

**声明**

```lepus
int64 eprintln(String format, Any args...);
```

**行为**

格式化写入标准错误流并追加换行。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `format` | `String format` | 格式模板，使用 {} 占位符 | 无 |
| `args` | `Any args...` | 实参列表 | 无 |

**返回值**

`int64`。格式化写入标准错误流并追加换行。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
eprintln(format, args);
```

**源码**：[source/semantics/formatted_output.cpp:15](../../source/semantics/formatted_output.cpp#L15)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-168"></a>

### format（169）

**声明**

```lepus
String format(String format, Any args...);
```

**行为**

返回格式化的新字符串。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `format` | `String format` | 格式模板，使用 {} 占位符 | 无 |
| `args` | `Any args...` | 实参列表 | 无 |

**返回值**

`String`。返回格式化的新字符串。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
format(format, args);
```

**源码**：[source/semantics/boxing.cpp:1361](../../source/semantics/boxing.cpp#L1361)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-169"></a>

### putChar（170）

**声明**

```lepus
int64 putChar(char value);
```

**行为**

输出一个字符。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `char value` | 输入值或写入的新值 | 无 |

**返回值**

`int64`。输出一个字符。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
putChar(value);
```

**源码**：[source/semantics/boxing.cpp:1407](../../source/semantics/boxing.cpp#L1407)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-170"></a>

### pause（171）

**声明**

```lepus
int64 pause();
```

**行为**

调试暂停，提示写入标准错误；非交互输入立即返回。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`int64`。调试暂停，提示写入标准错误；非交互输入立即返回。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
pause();
```

**源码**：[source/semantics/boxing.cpp:1368](../../source/semantics/boxing.cpp#L1368)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-171"></a>

### inputLine（172）

**声明**

```lepus
String inputLine();
```

**行为**

读取下一项原文，输入项按空白分隔，不是整行。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`String`。读取下一项原文，输入项按空白分隔，不是整行。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
inputLine();
```

**源码**：[source/semantics/boxing.cpp:1417](../../source/semantics/boxing.cpp#L1417)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-172"></a>

### inputInt（173）

**声明**

```lepus
int64 inputInt();
```

**行为**

读取下一项有符号整数。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`int64`。读取下一项有符号整数。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
inputInt();
```

**源码**：[source/semantics/boxing.cpp:1417](../../source/semantics/boxing.cpp#L1417)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-173"></a>

### inputUInt（174）

**声明**

```lepus
uint64 inputUInt();
```

**行为**

读取下一项无符号整数。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`uint64`。读取下一项无符号整数。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
inputUInt();
```

**源码**：[source/semantics/boxing.cpp:1417](../../source/semantics/boxing.cpp#L1417)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-174"></a>

### inputFloat（175）

**声明**

```lepus
float64 inputFloat();
```

**行为**

读取下一项浮点数。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`float64`。读取下一项浮点数。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
inputFloat();
```

**源码**：[source/semantics/boxing.cpp:1417](../../source/semantics/boxing.cpp#L1417)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-175"></a>

### inputBool（176）

**声明**

```lepus
bool inputBool();
```

**行为**

读取下一项布尔值。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`bool`。读取下一项布尔值。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
inputBool();
```

**源码**：[source/semantics/boxing.cpp:1417](../../source/semantics/boxing.cpp#L1417)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-176"></a>

### inputChar（177）

**声明**

```lepus
char inputChar();
```

**行为**

读取下一项字符。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`char`。读取下一项字符。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
inputChar();
```

**源码**：[source/semantics/boxing.cpp:1417](../../source/semantics/boxing.cpp#L1417)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-177"></a>

### inputBigInteger（178）

**声明**

```lepus
BigInteger inputBigInteger();
```

**行为**

读取下一项任意精度整数。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`BigInteger`。读取下一项任意精度整数。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
inputBigInteger();
```

**源码**：[source/semantics/boxing.cpp:1417](../../source/semantics/boxing.cpp#L1417)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-178"></a>

### inputBigDecimal（179）

**声明**

```lepus
BigDecimal inputBigDecimal();
```

**行为**

读取下一项任意精度小数。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`BigDecimal`。读取下一项任意精度小数。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
inputBigDecimal();
```

**源码**：[source/semantics/boxing.cpp:1417](../../source/semantics/boxing.cpp#L1417)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-179"></a>

### panic（180）

**声明**

```lepus
int64 panic(String message);
```

**行为**

终止当前执行并报告 message。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `message` | `String message` | 诊断文本或消息载荷 | 无 |

**返回值**

`int64`。终止当前执行并报告 message。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
panic(message);
```

**源码**：[source/semantics/boxing.cpp:1436](../../source/semantics/boxing.cpp#L1436)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-180"></a>

### len（181）

**声明**

```lepus
int64 len(Any value);
```

**行为**

返回当前元素数量。

返回字符串字节长度、数组元素数量或容器长度。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `Any value` | 输入值或写入的新值 | 无 |

**返回值**

`int64`。返回当前元素数量。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
len(value);
```

**源码**：[source/semantics/boxing.cpp:787](../../source/semantics/boxing.cpp#L787)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-181"></a>

### toReference（182）

**声明**

```lepus
T mut& toReference<T>(Option<T mut&> value);
```

**行为**

将可选可写引用转换成可写引用；None 无法转为有效引用。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `Option<T mut&> value` | 输入值或写入的新值 | 无 |

**返回值**

`T mut&`。将可选可写引用转换成可写引用；None 无法转为有效引用。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
toReference<T>(value);
```

**源码**：[source/lepus_generic.cpp:2220](../../source/lepus_generic.cpp#L2220)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-182"></a>

### toOptional（183）

**声明**

```lepus
Option<T mut&> toOptional<T>(T mut& value);
```

**行为**

将可写引用封装成可选引用，不复制指向的对象。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T mut& value` | 输入值或写入的新值 | 无 |

**返回值**

`Option<T mut&>`。将可写引用封装成可选引用，不复制指向的对象。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
toOptional<T>(value);
```

**源码**：[source/lepus_generic.cpp:2226](../../source/lepus_generic.cpp#L2226)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-183"></a>

### memcpy（184）

**声明**

```lepus
int64 memcpy(char mut& destination, char const& source, int64 bytes);
```

**行为**

按字节复制内存；调用者保证源与目标存储范围有效。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `destination` | `char mut& destination` | 可写目标存储首地址 | 无 |
| `source` | `char const& source` | 输入游标、存储或事件来源值 | 无 |
| `bytes` | `int64 bytes` | 输入或输出字节存储 | 无 |

**返回值**

`int64`。按字节复制内存；调用者保证源与目标存储范围有效。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
memcpy(destination, source, bytes);
```

**源码**：[source/semantics/boxing.cpp:1453](../../source/semantics/boxing.cpp#L1453)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-184"></a>

### memset（185）

**声明**

```lepus
int64 memset(char mut& destination, int64 value, int64 bytes);
```

**行为**

按字节填充存储；调用者保证目标范围有效。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `destination` | `char mut& destination` | 可写目标存储首地址 | 无 |
| `value` | `int64 value` | 输入值或写入的新值 | 无 |
| `bytes` | `int64 bytes` | 输入或输出字节存储 | 无 |

**返回值**

`int64`。按字节填充存储；调用者保证目标范围有效。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
memset(destination, value, bytes);
```

**源码**：[source/semantics/boxing.cpp:1453](../../source/semantics/boxing.cpp#L1453)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-185"></a>

### List::copy（186）

**声明**

```lepus
List<T> copy();
```

**行为**

返回独立的值副本。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`List<T>`。返回独立的值副本。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.copy();
```

**源码**：[source/semantics/aggregate_ownership.cpp:257](../../source/semantics/aggregate_ownership.cpp#L257)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-186"></a>

### List::push（187）

**声明**

```lepus
int64 push(T value);
```

**行为**

将 value 追加到列表尾部。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`int64`。成功时返回 0；错误通过运行时诊断报告，该返回值不是成功标志。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.push(value);
```

**源码**：[source/semantics/aggregate_ownership.cpp:258](../../source/semantics/aggregate_ownership.cpp#L258)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-187"></a>

### List::insert（188）

**声明**

```lepus
int64 insert(int64 index, T value);
```

**行为**

在 index 之前插入 value。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `index` | `int64 index` | 从 0 开始的位置 | 无 |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`int64`。成功时返回 0；错误通过运行时诊断报告，该返回值不是成功标志。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.insert(index, value);
```

**源码**：[source/semantics/aggregate_ownership.cpp:259](../../source/semantics/aggregate_ownership.cpp#L259)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-188"></a>

### List::at（189）

**声明**

```lepus
T at(int64 index);
```

**行为**

读取从 0 开始的指定位置。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `index` | `int64 index` | 从 0 开始的位置 | 无 |

**返回值**

`T`。读取从 0 开始的指定位置。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.at(index);
```

**源码**：[source/semantics/aggregate_ownership.cpp:260](../../source/semantics/aggregate_ownership.cpp#L260)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-189"></a>

### List::pop（190）

**声明**

```lepus
T pop();
```

**行为**

移除列表尾元素并返回值，空列表时报错。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`T`。移除列表尾元素并返回值，空列表时报错。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.pop();
```

**源码**：[source/semantics/aggregate_ownership.cpp:261](../../source/semantics/aggregate_ownership.cpp#L261)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-190"></a>

### List::remove（191）

**声明**

```lepus
int64 remove(int64 index);
```

**行为**

删除指定位置或键的元素。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `index` | `int64 index` | 从 0 开始的位置 | 无 |

**返回值**

`int64`。成功时返回 0；错误通过运行时诊断报告，该返回值不是成功标志。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.remove(index);
```

**源码**：[source/semantics/aggregate_ownership.cpp:262](../../source/semantics/aggregate_ownership.cpp#L262)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-191"></a>

### List::indexOf（192）

**声明**

```lepus
int64 indexOf(T value);
```

**行为**

返回首次出现的下标；未找到返回 -1。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`int64`。返回首次出现的下标；未找到返回 -1。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.indexOf(value);
```

**源码**：[source/semantics/aggregate_ownership.cpp:263](../../source/semantics/aggregate_ownership.cpp#L263)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-192"></a>

### List::contains（193）

**声明**

```lepus
bool contains(T value);
```

**行为**

判断是否包含目标值或匹配文本。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`bool`。判断是否包含目标值或匹配文本。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.contains(value);
```

**源码**：[source/semantics/aggregate_ownership.cpp:264](../../source/semantics/aggregate_ownership.cpp#L264)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-193"></a>

### List::has（194）

**声明**

```lepus
bool has(T value);
```

**行为**

检查是否包含指定值或键。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`bool`。检查是否包含指定值或键。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.has(value);
```

**源码**：[source/semantics/aggregate_ownership.cpp:265](../../source/semantics/aggregate_ownership.cpp#L265)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-194"></a>

### List::clear（195）

**声明**

```lepus
int64 clear();
```

**行为**

移除全部元素。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`int64`。成功时返回 0；错误通过运行时诊断报告，该返回值不是成功标志。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.clear();
```

**源码**：[source/semantics/aggregate_ownership.cpp:266](../../source/semantics/aggregate_ownership.cpp#L266)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-195"></a>

### List::len（196）

**声明**

```lepus
int64 len();
```

**行为**

返回当前元素数量。

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

**源码**：[source/semantics/aggregate_ownership.cpp:267](../../source/semantics/aggregate_ownership.cpp#L267)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-196"></a>

### Set::copy（197）

**声明**

```lepus
Set<T> copy();
```

**行为**

返回独立的值副本。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`Set<T>`。返回独立的值副本。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.copy();
```

**源码**：[source/semantics/aggregate_ownership.cpp:257](../../source/semantics/aggregate_ownership.cpp#L257)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-197"></a>

### Set::add（198）

**声明**

```lepus
int64 add(T value);
```

**行为**

计算逐项相加或追加元素，具体操作取决于所属类型。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`int64`。成功时返回 0；错误通过运行时诊断报告，该返回值不是成功标志。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.add(value);
```

**源码**：[source/semantics/aggregate_ownership.cpp:275](../../source/semantics/aggregate_ownership.cpp#L275)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-198"></a>

### Set::has（199）

**声明**

```lepus
int64 has(T value);
```

**行为**

检查是否包含指定值或键。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`int64`。存在返回 1，不存在返回 0。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.has(value);
```

**源码**：[source/semantics/aggregate_ownership.cpp:265](../../source/semantics/aggregate_ownership.cpp#L265)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-199"></a>

### Set::contains（200）

**声明**

```lepus
int64 contains(T value);
```

**行为**

判断是否包含目标值或匹配文本。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`int64`。存在返回 1，不存在返回 0。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.contains(value);
```

**源码**：[source/semantics/aggregate_ownership.cpp:264](../../source/semantics/aggregate_ownership.cpp#L264)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-200"></a>

### Set::remove（201）

**声明**

```lepus
int64 remove(T value);
```

**行为**

删除指定位置或键的元素。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`int64`。找到并删除返回 1，键不存在返回 0。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.remove(value);
```

**源码**：[source/semantics/aggregate_ownership.cpp:262](../../source/semantics/aggregate_ownership.cpp#L262)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-201"></a>

### Set::at（202）

**声明**

```lepus
T at(int64 index);
```

**行为**

读取从 0 开始的指定位置。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `index` | `int64 index` | 从 0 开始的位置 | 无 |

**返回值**

`T`。读取从 0 开始的指定位置。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.at(index);
```

**源码**：[source/semantics/aggregate_ownership.cpp:260](../../source/semantics/aggregate_ownership.cpp#L260)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-202"></a>

### Set::clear（203）

**声明**

```lepus
int64 clear();
```

**行为**

移除全部元素。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`int64`。成功时返回 0；错误通过运行时诊断报告，该返回值不是成功标志。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.clear();
```

**源码**：[source/semantics/aggregate_ownership.cpp:266](../../source/semantics/aggregate_ownership.cpp#L266)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-203"></a>

### Set::len（204）

**声明**

```lepus
int64 len();
```

**行为**

返回当前元素数量。

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

**源码**：[source/semantics/aggregate_ownership.cpp:267](../../source/semantics/aggregate_ownership.cpp#L267)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-204"></a>

### Map::copy（205）

**声明**

```lepus
Map<K,V> copy();
```

**行为**

返回独立的值副本。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`Map<K,V>`。返回独立的值副本。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.copy();
```

**源码**：[source/semantics/aggregate_ownership.cpp:257](../../source/semantics/aggregate_ownership.cpp#L257)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-205"></a>

### Map::put（206）

**声明**

```lepus
int64 put(K key,V value);
```

**行为**

写入 key 对应的 value，已有键被替换。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `key` | `K key` | 查找键或下标 | 无 |
| `value` | `V value` | 输入值或写入的新值 | 无 |

**返回值**

`int64`。成功时返回 0；错误通过运行时诊断报告，该返回值不是成功标志。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.put(key, value);
```

**源码**：[source/semantics/aggregate_ownership.cpp:289](../../source/semantics/aggregate_ownership.cpp#L289)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-206"></a>

### Map::has（207）

**声明**

```lepus
int64 has(K key);
```

**行为**

检查是否包含指定值或键。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `key` | `K key` | 查找键或下标 | 无 |

**返回值**

`int64`。存在返回 1，不存在返回 0。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.has(key);
```

**源码**：[source/semantics/aggregate_ownership.cpp:265](../../source/semantics/aggregate_ownership.cpp#L265)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-207"></a>

### Map::contains（208）

**声明**

```lepus
int64 contains(K key);
```

**行为**

判断是否包含目标值或匹配文本。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `key` | `K key` | 查找键或下标 | 无 |

**返回值**

`int64`。存在返回 1，不存在返回 0。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.contains(key);
```

**源码**：[source/semantics/aggregate_ownership.cpp:264](../../source/semantics/aggregate_ownership.cpp#L264)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-208"></a>

### Map::remove（209）

**声明**

```lepus
int64 remove(K key);
```

**行为**

删除指定位置或键的元素。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `key` | `K key` | 查找键或下标 | 无 |

**返回值**

`int64`。找到并删除返回 1，键不存在返回 0。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.remove(key);
```

**源码**：[source/semantics/aggregate_ownership.cpp:262](../../source/semantics/aggregate_ownership.cpp#L262)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-209"></a>

### Map::keyAt（210）

**声明**

```lepus
K keyAt(int64 index);
```

**行为**

按遍历位置读取键。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `index` | `int64 index` | 从 0 开始的位置 | 无 |

**返回值**

`K`。按遍历位置读取键。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.keyAt(index);
```

**源码**：[source/semantics/aggregate_ownership.cpp:293](../../source/semantics/aggregate_ownership.cpp#L293)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-210"></a>

### Map::valAt（211）

**声明**

```lepus
V valAt(int64 index);
```

**行为**

按遍历位置读取值。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `index` | `int64 index` | 从 0 开始的位置 | 无 |

**返回值**

`V`。按遍历位置读取值。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.valAt(index);
```

**源码**：[source/semantics/aggregate_ownership.cpp:294](../../source/semantics/aggregate_ownership.cpp#L294)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-211"></a>

### Map::clear（212）

**声明**

```lepus
int64 clear();
```

**行为**

移除全部元素。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`int64`。成功时返回 0；错误通过运行时诊断报告，该返回值不是成功标志。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.clear();
```

**源码**：[source/semantics/aggregate_ownership.cpp:266](../../source/semantics/aggregate_ownership.cpp#L266)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-212"></a>

### Map::len（213）

**声明**

```lepus
int64 len();
```

**行为**

返回当前元素数量。

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

**源码**：[source/semantics/aggregate_ownership.cpp:267](../../source/semantics/aggregate_ownership.cpp#L267)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-213"></a>

### Any::copy（214）

**声明**

```lepus
Any copy();
```

**行为**

返回独立的值副本。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`Any`。返回独立的值副本。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.copy();
```

**源码**：[source/semantics/boxing.cpp:123](../../source/semantics/boxing.cpp#L123)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-214"></a>

### Any::isEmpty（215）

**声明**

```lepus
bool isEmpty();
```

**行为**

判断 Any 是否为空盒子，不公开内部执行期标签。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`bool`。判断 Any 是否为空盒子，不公开内部执行期标签。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.isEmpty();
```

**源码**：[source/semantics/boxing.cpp:1040](../../source/semantics/boxing.cpp#L1040)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-215"></a>

### Any::clear（216）

**声明**

```lepus
void clear();
```

**行为**

释放盒子拥有的载荷并恢复空状态；保留盒子自身的键身份。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

无返回值。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.clear();
```

**源码**：[source/semantics/boxing.cpp:1047](../../source/semantics/boxing.cpp#L1047)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-216"></a>

### Any::set（217）

**声明**

```lepus
int64 set(T value);
```

**行为**

以 value 的值副本和类型编号替换盒子内容。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`int64`。以 value 的值副本和类型编号替换盒子内容。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.set(value);
```

**源码**：[source/semantics/boxing.cpp:1063](../../source/semantics/boxing.cpp#L1063)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-217"></a>

### Any::getValue（218）

**声明**

```lepus
T getValue();
```

**行为**

按接收上下文要求的类型取出拥有值，类型不匹配时报错。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`T`。按接收上下文要求的类型取出拥有值，类型不匹配时报错。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.getValue();
```

**源码**：[source/semantics/boxing.cpp:164](../../source/semantics/boxing.cpp#L164)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-218"></a>

### Any::getReference（219）

**声明**

```lepus
T mut& getReference();
```

**行为**

按上下文要求的类型借用盒子内的值，引用受盒子生命周期约束。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`T mut&`。按上下文要求的类型借用盒子内的值，引用受盒子生命周期约束。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.getReference();
```

**源码**：[source/semantics/boxing.cpp:164](../../source/semantics/boxing.cpp#L164)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-219"></a>

### Any::getOptional（220）

**声明**

```lepus
Option<T mut&> getOptional();
```

**行为**

按上下文要求的类型获取可选引用，类型不匹配为 None。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`Option<T mut&>`。按上下文要求的类型获取可选引用，类型不匹配为 None。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.getOptional();
```

**源码**：[source/semantics/boxing.cpp:164](../../source/semantics/boxing.cpp#L164)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-220"></a>

### eq（221）

**声明**

```lepus
bool eq(T left,T right);
```

**行为**

内建运算函数，等价于 left == right。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `left` | `T left` | 左操作数或区间左端 | 无 |
| `right` | `T right` | 右操作数或区间右端 | 无 |

**返回值**

`bool`。内建运算函数，等价于 left == right。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
eq(left, right);
```

**源码**：[source/semantics/boxing.cpp:1](../../source/semantics/boxing.cpp#L1)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-221"></a>

### ne（222）

**声明**

```lepus
bool ne(T left,T right);
```

**行为**

内建运算函数，等价于 left != right。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `left` | `T left` | 左操作数或区间左端 | 无 |
| `right` | `T right` | 右操作数或区间右端 | 无 |

**返回值**

`bool`。内建运算函数，等价于 left != right。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
ne(left, right);
```

**源码**：[source/semantics/boxing.cpp:1](../../source/semantics/boxing.cpp#L1)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-222"></a>

### lt（223）

**声明**

```lepus
bool lt(T left,T right);
```

**行为**

内建运算函数，等价于 left < right。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `left` | `T left` | 左操作数或区间左端 | 无 |
| `right` | `T right` | 右操作数或区间右端 | 无 |

**返回值**

`bool`。内建运算函数，等价于 left < right。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
lt(left, right);
```

**源码**：[source/semantics/boxing.cpp:1](../../source/semantics/boxing.cpp#L1)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-223"></a>

### le（224）

**声明**

```lepus
bool le(T left,T right);
```

**行为**

内建运算函数，等价于 left <= right。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `left` | `T left` | 左操作数或区间左端 | 无 |
| `right` | `T right` | 右操作数或区间右端 | 无 |

**返回值**

`bool`。内建运算函数，等价于 left <= right。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
le(left, right);
```

**源码**：[source/semantics/boxing.cpp:1](../../source/semantics/boxing.cpp#L1)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-224"></a>

### gt（225）

**声明**

```lepus
bool gt(T left,T right);
```

**行为**

内建运算函数，等价于 left > right。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `left` | `T left` | 左操作数或区间左端 | 无 |
| `right` | `T right` | 右操作数或区间右端 | 无 |

**返回值**

`bool`。内建运算函数，等价于 left > right。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
gt(left, right);
```

**源码**：[source/semantics/boxing.cpp:1](../../source/semantics/boxing.cpp#L1)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-225"></a>

### ge（226）

**声明**

```lepus
bool ge(T left,T right);
```

**行为**

内建运算函数，等价于 left >= right。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `left` | `T left` | 左操作数或区间左端 | 无 |
| `right` | `T right` | 右操作数或区间右端 | 无 |

**返回值**

`bool`。内建运算函数，等价于 left >= right。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
ge(left, right);
```

**源码**：[source/semantics/boxing.cpp:1](../../source/semantics/boxing.cpp#L1)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-226"></a>

### add（227）

**声明**

```lepus
R add(T left,U right);
```

**行为**

计算逐项相加或追加元素，具体操作取决于所属类型。

内建运算函数，等价于 left + right，结果由运算符解析决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `left` | `T left` | 左操作数或区间左端 | 无 |
| `right` | `U right` | 右操作数或区间右端 | 无 |

**返回值**

`R`。计算逐项相加或追加元素，具体操作取决于所属类型。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
add(left, right);
```

**源码**：[source/semantics/boxing.cpp:1221](../../source/semantics/boxing.cpp#L1221)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-227"></a>

### sub（228）

**声明**

```lepus
R sub(T left,U right);
```

**行为**

内建运算函数，等价于 left - right。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `left` | `T left` | 左操作数或区间左端 | 无 |
| `right` | `U right` | 右操作数或区间右端 | 无 |

**返回值**

`R`。内建运算函数，等价于 left - right。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
sub(left, right);
```

**源码**：[source/semantics/boxing.cpp:1](../../source/semantics/boxing.cpp#L1)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-228"></a>

### mul（229）

**声明**

```lepus
R mul(T left,U right);
```

**行为**

内建运算函数，等价于 left * right。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `left` | `T left` | 左操作数或区间左端 | 无 |
| `right` | `U right` | 右操作数或区间右端 | 无 |

**返回值**

`R`。内建运算函数，等价于 left * right。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
mul(left, right);
```

**源码**：[source/semantics/boxing.cpp:1](../../source/semantics/boxing.cpp#L1)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-229"></a>

### div（230）

**声明**

```lepus
R div(T left,U right);
```

**行为**

内建运算函数，等价于 left / right。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `left` | `T left` | 左操作数或区间左端 | 无 |
| `right` | `U right` | 右操作数或区间右端 | 无 |

**返回值**

`R`。内建运算函数，等价于 left / right。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
div(left, right);
```

**源码**：[source/semantics/boxing.cpp:1](../../source/semantics/boxing.cpp#L1)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-230"></a>

### mod（231）

**声明**

```lepus
R mod(T left,U right);
```

**行为**

内建运算函数，等价于 left % right。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `left` | `T left` | 左操作数或区间左端 | 无 |
| `right` | `U right` | 右操作数或区间右端 | 无 |

**返回值**

`R`。内建运算函数，等价于 left % right。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
mod(left, right);
```

**源码**：[source/semantics/boxing.cpp:923](../../source/semantics/boxing.cpp#L923)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-231"></a>

### index（232）

**声明**

```lepus
T index(T const& data,int64 index);
```

**行为**

读取 key 对应的元素，供下标语法调用。

读取连续存储的第 index 项，相当于下标操作。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `data` | `T const& data` | 连续存储的首元素引用，调用者保证范围有效 | 无 |
| `index` | `int64 index` | 从 0 开始的位置 | 无 |

**返回值**

`T`。读取 key 对应的元素，供下标语法调用。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
std::core::index(data, index);
```

**源码**：[source/semantics/boxing.cpp:765](../../source/semantics/boxing.cpp#L765)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-232"></a>

### Add::leftAdd（233）

**声明**

```lepus
R leftAdd(T value);
```

**行为**

用户类型实现此运算符接口时提供 leftAdd；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 leftAdd；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.leftAdd(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-233"></a>

### RightAdd::rightAdd（234）

**声明**

```lepus
R rightAdd(T value);
```

**行为**

用户类型实现此运算符接口时提供 rightAdd；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 rightAdd；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.rightAdd(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-234"></a>

### Subtract::leftSub（235）

**声明**

```lepus
R leftSub(T value);
```

**行为**

用户类型实现此运算符接口时提供 leftSub；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 leftSub；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.leftSub(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-235"></a>

### RightSubtract::rightSub（236）

**声明**

```lepus
R rightSub(T value);
```

**行为**

用户类型实现此运算符接口时提供 rightSub；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 rightSub；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.rightSub(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-236"></a>

### Multiply::leftMul（237）

**声明**

```lepus
R leftMul(T value);
```

**行为**

用户类型实现此运算符接口时提供 leftMul；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 leftMul；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.leftMul(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-237"></a>

### RightMultiply::rightMul（238）

**声明**

```lepus
R rightMul(T value);
```

**行为**

用户类型实现此运算符接口时提供 rightMul；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 rightMul；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.rightMul(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-238"></a>

### Divide::leftDiv（239）

**声明**

```lepus
R leftDiv(T value);
```

**行为**

用户类型实现此运算符接口时提供 leftDiv；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 leftDiv；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.leftDiv(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-239"></a>

### RightDivide::rightDiv（240）

**声明**

```lepus
R rightDiv(T value);
```

**行为**

用户类型实现此运算符接口时提供 rightDiv；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 rightDiv；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.rightDiv(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-240"></a>

### Modulo::leftMod（241）

**声明**

```lepus
R leftMod(T value);
```

**行为**

用户类型实现此运算符接口时提供 leftMod；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 leftMod；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.leftMod(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-241"></a>

### RightModulo::rightMod（242）

**声明**

```lepus
R rightMod(T value);
```

**行为**

用户类型实现此运算符接口时提供 rightMod；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 rightMod；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.rightMod(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-242"></a>

### Equal::leftEqual（243）

**声明**

```lepus
R leftEqual(T value);
```

**行为**

用户类型实现此运算符接口时提供 leftEqual；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 leftEqual；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.leftEqual(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-243"></a>

### RightEqual::rightEqual（244）

**声明**

```lepus
R rightEqual(T value);
```

**行为**

用户类型实现此运算符接口时提供 rightEqual；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 rightEqual；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.rightEqual(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-244"></a>

### NotEqual::leftNotEqual（245）

**声明**

```lepus
R leftNotEqual(T value);
```

**行为**

用户类型实现此运算符接口时提供 leftNotEqual；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 leftNotEqual；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.leftNotEqual(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-245"></a>

### RightNotEqual::rightNotEqual（246）

**声明**

```lepus
R rightNotEqual(T value);
```

**行为**

用户类型实现此运算符接口时提供 rightNotEqual；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 rightNotEqual；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.rightNotEqual(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-246"></a>

### Less::leftLess（247）

**声明**

```lepus
R leftLess(T value);
```

**行为**

用户类型实现此运算符接口时提供 leftLess；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 leftLess；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.leftLess(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-247"></a>

### RightLess::rightLess（248）

**声明**

```lepus
R rightLess(T value);
```

**行为**

用户类型实现此运算符接口时提供 rightLess；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 rightLess；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.rightLess(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-248"></a>

### LessOrEqual::leftLessEqual（249）

**声明**

```lepus
R leftLessEqual(T value);
```

**行为**

用户类型实现此运算符接口时提供 leftLessEqual；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 leftLessEqual；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.leftLessEqual(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-249"></a>

### RightLessOrEqual::rightLessEqual（250）

**声明**

```lepus
R rightLessEqual(T value);
```

**行为**

用户类型实现此运算符接口时提供 rightLessEqual；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 rightLessEqual；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.rightLessEqual(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-250"></a>

### Greater::leftGreater（251）

**声明**

```lepus
R leftGreater(T value);
```

**行为**

用户类型实现此运算符接口时提供 leftGreater；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 leftGreater；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.leftGreater(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-251"></a>

### RightGreater::rightGreater（252）

**声明**

```lepus
R rightGreater(T value);
```

**行为**

用户类型实现此运算符接口时提供 rightGreater；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 rightGreater；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.rightGreater(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-252"></a>

### GreaterOrEqual::leftGreaterEqual（253）

**声明**

```lepus
R leftGreaterEqual(T value);
```

**行为**

用户类型实现此运算符接口时提供 leftGreaterEqual；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 leftGreaterEqual；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.leftGreaterEqual(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-253"></a>

### RightGreaterOrEqual::rightGreaterEqual（254）

**声明**

```lepus
R rightGreaterEqual(T value);
```

**行为**

用户类型实现此运算符接口时提供 rightGreaterEqual；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 rightGreaterEqual；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.rightGreaterEqual(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-254"></a>

### And::leftBitAnd（255）

**声明**

```lepus
R leftBitAnd(T value);
```

**行为**

用户类型实现此运算符接口时提供 leftBitAnd；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 leftBitAnd；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.leftBitAnd(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-255"></a>

### RightAnd::rightBitAnd（256）

**声明**

```lepus
R rightBitAnd(T value);
```

**行为**

用户类型实现此运算符接口时提供 rightBitAnd；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 rightBitAnd；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.rightBitAnd(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-256"></a>

### Or::leftBitOr（257）

**声明**

```lepus
R leftBitOr(T value);
```

**行为**

用户类型实现此运算符接口时提供 leftBitOr；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 leftBitOr；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.leftBitOr(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-257"></a>

### RightOr::rightBitOr（258）

**声明**

```lepus
R rightBitOr(T value);
```

**行为**

用户类型实现此运算符接口时提供 rightBitOr；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 rightBitOr；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.rightBitOr(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-258"></a>

### Xor::leftBitXor（259）

**声明**

```lepus
R leftBitXor(T value);
```

**行为**

用户类型实现此运算符接口时提供 leftBitXor；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 leftBitXor；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.leftBitXor(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-259"></a>

### RightXor::rightBitXor（260）

**声明**

```lepus
R rightBitXor(T value);
```

**行为**

用户类型实现此运算符接口时提供 rightBitXor；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 rightBitXor；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.rightBitXor(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-260"></a>

### ShiftLeft::leftShiftLeft（261）

**声明**

```lepus
R leftShiftLeft(T value);
```

**行为**

用户类型实现此运算符接口时提供 leftShiftLeft；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 leftShiftLeft；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.leftShiftLeft(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-261"></a>

### RightShiftLeft::rightShiftLeft（262）

**声明**

```lepus
R rightShiftLeft(T value);
```

**行为**

用户类型实现此运算符接口时提供 rightShiftLeft；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 rightShiftLeft；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.rightShiftLeft(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-262"></a>

### ShiftRight::leftShiftRight（263）

**声明**

```lepus
R leftShiftRight(T value);
```

**行为**

用户类型实现此运算符接口时提供 leftShiftRight；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 leftShiftRight；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.leftShiftRight(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-263"></a>

### RightShiftRight::rightShiftRight（264）

**声明**

```lepus
R rightShiftRight(T value);
```

**行为**

用户类型实现此运算符接口时提供 rightShiftRight；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 rightShiftRight；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.rightShiftRight(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-264"></a>

### Negate::negate（265）

**声明**

```lepus
R negate(T value);
```

**行为**

用户类型实现此运算符接口时提供 negate；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 negate；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.negate(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-265"></a>

### BitNot::bitNot（266）

**声明**

```lepus
R bitNot(T value);
```

**行为**

用户类型实现此运算符接口时提供 bitNot；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 bitNot；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.bitNot(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-266"></a>

### Positive::positive（267）

**声明**

```lepus
R positive(T value);
```

**行为**

用户类型实现此运算符接口时提供 positive；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`R`。用户类型实现此运算符接口时提供 positive；R 为运算结果类型，T 为操作数类型，左右方向由接口名决定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.positive(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-267"></a>

### Hash::hash（268）

**声明**

```lepus
uint64 hash(T value);
```

**行为**

返回输入值的哈希值，由实现者保持与相等性的一致性。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `T value` | 输入值或写入的新值 | 无 |

**返回值**

`uint64`。返回输入值的哈希值，由实现者保持与相等性的一致性。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.hash(value);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-268"></a>

### Copy::copy（269）

**声明**

```lepus
T copy();
```

**行为**

返回独立的值副本。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`T`。返回独立的值副本。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.copy();
```

**源码**：[source/lepus_generic.cpp:2203](../../source/lepus_generic.cpp#L2203)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-269"></a>

### ExplicitConvert::explicitConvert（270）

**声明**

```lepus
T explicitConvert();
```

**行为**

由用户类型提供显式目标类型转换。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`T`。由用户类型提供显式目标类型转换。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.explicitConvert();
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-270"></a>

### ImplicitConvert::implicitConvert（271）

**声明**

```lepus
T implicitConvert();
```

**行为**

由用户类型提供隐式目标类型转换。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`T`。由用户类型提供隐式目标类型转换。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.implicitConvert();
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-271"></a>

### Index::index（272）

**声明**

```lepus
R index(T key);
```

**行为**

读取 key 对应的元素，供下标语法调用。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `key` | `T key` | 查找键或下标 | 无 |

**返回值**

`R`。读取 key 对应的元素，供下标语法调用。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.index(key);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-272"></a>

### Iterator::iterator（273）

**声明**

```lepus
Option<R> iterator(int64 index);
```

**行为**

读取指定迭代位置；无元素时返回 None。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `index` | `int64 index` | 从 0 开始的位置 | 无 |

**返回值**

`Option<R>`。读取指定迭代位置；无元素时返回 None。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.iterator(index);
```

**源码**：[source/lepus_generic.cpp:827](../../source/lepus_generic.cpp#L827)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-273"></a>

### Result::mapErr（274）

**声明**

```lepus
Result<T,F> mapErr<F>(F fn(E) mapper);
```

**行为**

仅在失败时调用 mapper 转换错误类型，成功值保持不变。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `mapper` | `F fn(E) mapper` | 转换回调，签名决定输入和输出类型 | 无 |

**返回值**

`Result<T,F>`。仅在失败时调用 mapper 转换错误类型，成功值保持不变。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.mapErr(mapper);
```

**源码**：[source/std/core.lep:2](../../source/std/core.lep#L2)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-274"></a>

### Option::okOr（275）

**声明**

```lepus
Result<T,E> okOr<E>(E error);
```

**行为**

将 None 转为给定错误，error 按普通实参规则求值。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `error` | `E error` | 缺失时产生的错误值。 | 无 |

**返回值**

`Result<T,E>`。将 None 转为给定错误，error 按普通实参规则求值。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.okOr(error);
```

**源码**：[source/std/core.lep:3](../../source/std/core.lep#L3)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-275"></a>

### Option::okOrElse（276）

**声明**

```lepus
Result<T,E> okOrElse<E>(E fn() factory);
```

**行为**

只在 None 时调用错误工厂，成功分支不调用。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `factory` | `E fn() factory` | 缺失时才调用的无参数错误工厂。 | 无 |

**返回值**

`Result<T,E>`。只在 None 时调用错误工厂，成功分支不调用。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.okOrElse(factory);
```

**源码**：[source/std/core.lep:3](../../source/std/core.lep#L3)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

<a id="api-276"></a>

### BarePtr::castTo（277）

**声明**

```lepus
unsafe BarePtr<U> castTo<U>();
```

**行为**



**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`BarePtr<U>`。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.castTo<U>();
```

**源码**：[source/std/core.lep:4](../../source/std/core.lep#L4)（编译器内建，签名为等价调用形式）。

**完整示例**：[example/samples/std_collections_algorithms.lep](../../example/samples/std_collections_algorithms.lep)。

## 补充完整示例

[example/samples/std_read_lines.lep](../../example/samples/std_read_lines.lep)。

```lepus
void main() {
    String first=readLine();
    String rest=readLines(3);
    println("[{}]",first);
    println("[{}]",rest);
    assert(readLines(0)=="");
    assert(readLine()=="");
}
```

## 补充完整示例

[example/samples/std_box.lep](../../example/samples/std_box.lep)。

```lepus
import std::macro;
import std::memory;

int64 closed = 0;
int64 calls = 0;

struct Resource : Closeable {
    int64 id;
    void close() { closed += self.id; }
};

int64 observed(int64 value) {
    calls += 1;
    return value;
}

void main() {
    assert($IN_RANGE(observed(4), observed(4), observed(6)));
    assert(calls == 3);
    assert($IN_RANGE(6, 4, 6));
    assert(!$IN_RANGE(7, 4, 6));
    assert(!$IN_RANGE(5, 6, 4));

    assert(Box<int64>::newBox(42).take() == 42);
    std::core::Box<int64> empty;
    assert(empty.isEmpty());
    empty.close();

    int64 before = Memory::stats().liveBlocks;
    {
        std::core::Box<List<int64>> numbers = Box<List<int64>>::newBox([2, 3]);
        List<int64> copy = numbers.get();
        copy.push(4);
        assert(numbers.get().len() == 2);
        List<int64> taken = numbers.take();
        assert(taken[0] == 2 && taken[1] == 3);
    }
    {
        Resource resource;
        resource.id = 7;
        let owned = $BOX(move resource);
        let moved = move owned;
        let extracted = moved.take();
        assert(extracted.id == 7 && closed == 0);
    }
    assert(closed == 7);
    {
        Resource resource;
        resource.id = 11;
        let owned = Box<Resource>::newBox(move resource);
    }
    assert(closed == 18);
    {
        Resource resource;
        resource.id = 13;
        let owned = std::core::Box<Resource>::newBox(move resource);
        owned.close();
        assert(owned.isEmpty());
        owned.close();
    }
    assert(closed == 31);
    assert(Memory::stats().liveBlocks == before);
    println("Box and standard macros OK");
}
```
