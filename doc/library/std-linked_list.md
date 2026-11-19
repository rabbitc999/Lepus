# std::linked_list — 双向链表

<!-- LANGUAGE NAVIGATION -->
[English（中文原文）](std-linked_list.md) · **简体中文**
<!-- END LANGUAGE NAVIGATION -->

[返回库目录](README.md) · [全部符号](symbols.md)

## 引入

```lepus
import std::linked_list;
```

## 公共约定与错误边界

LinkedList 以 Lepus 结构体保存双向节点数组、复用槽与单调游标索引，保留 Iterator<int64>。普通复制深复制节点并在副本内保留对应游标。首尾及按游标插删为平均 O(1)；按值查询和按序迭代 O(n)。游标不重复使用，删除或 clear 后永久失效；toArray 显式导出独立顺序快照。链表连接、插删、反转与游标管理均由 Lepus 实现，不注册宿主链表算法。

## 参考代码

聚合入口的具体操作参见对应子模块；本页类型/方法的调用形式列在下方。

## 类型、枚举、常量与字段

公开字段按源码原样列出；句柄和游标字段用于包装器状态，通常通过方法操作。枚举的括号表示变体载荷。

| 所属类型 | 声明 | 说明 |
| --- | --- | --- |
| 模块 | <a id="symbol-338"></a>`struct LinkedList: Iterator<int64>, Copy<LinkedList>` | 持久双向链表，值复制独立，旧游标在删除或 clear 后永久失效。 |

## 函数索引

| 名称 | 声明 |
| --- | --- |
| [LinkedList::create](#api-0) | `static LinkedList create()` |
| [LinkedList::copy](#api-1) | `LinkedList copy()` |
| [LinkedList::pushFront](#api-2) | `int64 pushFront(int64 value)` |
| [LinkedList::pushBack](#api-3) | `int64 pushBack(int64 value)` |
| [LinkedList::insertBefore](#api-4) | `int64 insertBefore(int64 cursor, int64 value)` |
| [LinkedList::insertAfter](#api-5) | `int64 insertAfter(int64 cursor, int64 value)` |
| [LinkedList::removeAt](#api-6) | `Option<int64> removeAt(int64 cursor)` |
| [LinkedList::popFront](#api-7) | `Option<int64> popFront()` |
| [LinkedList::popBack](#api-8) | `Option<int64> popBack()` |
| [LinkedList::reverse](#api-9) | `void reverse()` |
| [LinkedList::toArray](#api-10) | `List<int64> toArray()` |
| [LinkedList::fromArray](#api-11) | `static LinkedList fromArray(List<int64> items)` |
| [LinkedList::find](#api-12) | `int64 find(int64 value)` |
| [LinkedList::contains](#api-13) | `bool contains(int64 value)` |
| [LinkedList::front](#api-14) | `Option<int64> front()` |
| [LinkedList::back](#api-15) | `Option<int64> back()` |
| [LinkedList::len](#api-16) | `int64 len()` |
| [LinkedList::isEmpty](#api-17) | `bool isEmpty()` |
| [LinkedList::iterator](#api-18) | `Option<int64> iterator(int64 index)` |
| [LinkedList::clear](#api-19) | `void clear()` |

<a id="api-0"></a>

### LinkedList::create（1）

**声明**

```lepus
static LinkedList create();
```

**行为**

创建LinkedList，返回可供后续操作的值。

创建空链表。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`LinkedList`。创建LinkedList，返回可供后续操作的值。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
LinkedList::create();
```

**源码**：[source/std/linked_list.lep:17](../../source/std/linked_list.lep#L17)。

<a id="api-1"></a>

### LinkedList::copy（2）

**声明**

```lepus
LinkedList copy();
```

**行为**

返回独立的值副本。

取得独立副本，保留有效游标在副本中的对应关系。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`LinkedList`。返回独立的值副本。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.copy();
```

**源码**：[source/std/linked_list.lep:19](../../source/std/linked_list.lep#L19)。

<a id="api-2"></a>

### LinkedList::pushFront（3）

**声明**

```lepus
int64 pushFront(int64 value);
```

**行为**

将 value 插入首部。

头插并返回单调游标。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `int64 value` | 输入值或写入的新值 | 无 |

**返回值**

`int64`。将 value 插入首部。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.pushFront(value);
```

**源码**：[source/std/linked_list.lep:43](../../source/std/linked_list.lep#L43)。

<a id="api-3"></a>

### LinkedList::pushBack（4）

**声明**

```lepus
int64 pushBack(int64 value);
```

**行为**

将 value 追加到尾部。

尾插并返回单调游标。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `int64 value` | 输入值或写入的新值 | 无 |

**返回值**

`int64`。将 value 追加到尾部。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.pushBack(value);
```

**源码**：[source/std/linked_list.lep:45](../../source/std/linked_list.lep#L45)。

<a id="api-4"></a>

### LinkedList::insertBefore（5）

**声明**

```lepus
int64 insertBefore(int64 cursor, int64 value);
```

**行为**

在有效游标之前插入；无效游标返回 -1。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `cursor` | `int64 cursor` | 仍有效的节点游标 | 无 |
| `value` | `int64 value` | 输入值或写入的新值 | 无 |

**返回值**

`int64`。在有效游标之前插入；无效游标返回 -1。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.insertBefore(cursor, value);
```

**源码**：[source/std/linked_list.lep:47](../../source/std/linked_list.lep#L47)。

<a id="api-5"></a>

### LinkedList::insertAfter（6）

**声明**

```lepus
int64 insertAfter(int64 cursor, int64 value);
```

**行为**

在有效游标之后插入；无效游标返回 -1。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `cursor` | `int64 cursor` | 仍有效的节点游标 | 无 |
| `value` | `int64 value` | 输入值或写入的新值 | 无 |

**返回值**

`int64`。在有效游标之后插入；无效游标返回 -1。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.insertAfter(cursor, value);
```

**源码**：[source/std/linked_list.lep:53](../../source/std/linked_list.lep#L53)。

<a id="api-6"></a>

### LinkedList::removeAt（7）

**声明**

```lepus
Option<int64> removeAt(int64 cursor);
```

**行为**

摘除并回收节点槽，旧游标永久失效。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `cursor` | `int64 cursor` | 仍有效的节点游标 | 无 |

**返回值**

`Option<int64>`。摘除并回收节点槽，旧游标永久失效。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.removeAt(cursor);
```

**源码**：[source/std/linked_list.lep:59](../../source/std/linked_list.lep#L59)。

<a id="api-7"></a>

### LinkedList::popFront（8）

**声明**

```lepus
Option<int64> popFront();
```

**行为**

移除首部并返回 Option；空时 None。

弹出首节点，空链表返回 None。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`Option<int64>`。移除首部并返回 Option；空时 None。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.popFront();
```

**源码**：[source/std/linked_list.lep:73](../../source/std/linked_list.lep#L73)。

<a id="api-8"></a>

### LinkedList::popBack（9）

**声明**

```lepus
Option<int64> popBack();
```

**行为**

移除尾部并返回 Option；空时 None。

弹出尾节点，空链表返回 None。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`Option<int64>`。移除尾部并返回 Option；空时 None。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.popBack();
```

**源码**：[source/std/linked_list.lep:78](../../source/std/linked_list.lep#L78)。

<a id="api-9"></a>

### LinkedList::reverse（10）

**声明**

```lepus
void reverse();
```

**行为**

原地反转全部连接，不改变有效游标。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

无返回值。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.reverse();
```

**源码**：[source/std/linked_list.lep:83](../../source/std/linked_list.lep#L83)。

<a id="api-10"></a>

### LinkedList::toArray（11）

**声明**

```lepus
List<int64> toArray();
```

**行为**

复制成由调用者作用域管理的独立运行时数组。

按链表顺序导出独立值列表。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`List<int64>`。复制成由调用者作用域管理的独立运行时数组。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.toArray();
```

**源码**：[source/std/linked_list.lep:93](../../source/std/linked_list.lep#L93)。

<a id="api-11"></a>

### LinkedList::fromArray（12）

**声明**

```lepus
static LinkedList fromArray(List<int64> items);
```

**行为**

从连续数组复制元素；带 count 的重载由调用者保证有效范围。

从值列表建立独立链表。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `items` | `List<int64> items` | 参与处理的列表元素 | 无 |

**返回值**

`LinkedList`。从连续数组复制元素；带 count 的重载由调用者保证有效范围。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
LinkedList::fromArray(items);
```

**源码**：[source/std/linked_list.lep:99](../../source/std/linked_list.lep#L99)。

<a id="api-12"></a>

### LinkedList::find（13）

**声明**

```lepus
int64 find(int64 value);
```

**行为**

查找首个匹配项，返回下标或可选载荷；具体未找到表示见返回类型及模块约定。

查找首个相同值的游标，不存在时返回 -1。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `int64 value` | 输入值或写入的新值 | 无 |

**返回值**

`int64`。查找首个匹配项，返回下标或可选载荷；具体未找到表示见返回类型及模块约定。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.find(value);
```

**源码**：[source/std/linked_list.lep:105](../../source/std/linked_list.lep#L105)。

<a id="api-13"></a>

### LinkedList::contains（14）

**声明**

```lepus
bool contains(int64 value);
```

**行为**

判断是否包含目标值或匹配文本。

判断是否含有给定值。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `value` | `int64 value` | 输入值或写入的新值 | 无 |

**返回值**

`bool`。判断是否包含目标值或匹配文本。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.contains(value);
```

**源码**：[source/std/linked_list.lep:115](../../source/std/linked_list.lep#L115)。

<a id="api-14"></a>

### LinkedList::front（15）

**声明**

```lepus
Option<int64> front();
```

**行为**

取得首节点值，空链表返回 None。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`Option<int64>`。取得首节点值，空链表返回 None。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.front();
```

**源码**：[source/std/linked_list.lep:117](../../source/std/linked_list.lep#L117)。

<a id="api-15"></a>

### LinkedList::back（16）

**声明**

```lepus
Option<int64> back();
```

**行为**

取得尾节点值，空链表返回 None。

**参数**

无显式参数。实例方法使用当前接收者。

**返回值**

`Option<int64>`。取得尾节点值，空链表返回 None。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.back();
```

**源码**：[source/std/linked_list.lep:122](../../source/std/linked_list.lep#L122)。

<a id="api-16"></a>

### LinkedList::len（17）

**声明**

```lepus
int64 len();
```

**行为**

返回当前元素数量。

返回有效节点数。

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

**源码**：[source/std/linked_list.lep:127](../../source/std/linked_list.lep#L127)。

<a id="api-17"></a>

### LinkedList::isEmpty（18）

**声明**

```lepus
bool isEmpty();
```

**行为**

判断 Any 是否为空盒子，不公开内部执行期标签。

判断链表是否为空。

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

**源码**：[source/std/linked_list.lep:129](../../source/std/linked_list.lep#L129)。

<a id="api-18"></a>

### LinkedList::iterator（19）

**声明**

```lepus
Option<int64> iterator(int64 index);
```

**行为**

读取指定迭代位置；无元素时返回 None。

按顺序读取第 index 个值，越界返回 None。

**参数**

| 参数 | 类型／声明 | 含义 | 默认值 |
| --- | --- | --- | --- |
| `index` | `int64 index` | 从 0 开始的位置 | 无 |

**返回值**

`Option<int64>`。读取指定迭代位置；无元素时返回 None。

**错误与边界**

遵循本页“公共约定与错误边界”。

**调用形式**

```lepus
obj.iterator(index);
```

**源码**：[source/std/linked_list.lep:131](../../source/std/linked_list.lep#L131)。

<a id="api-19"></a>

### LinkedList::clear（20）

**声明**

```lepus
void clear();
```

**行为**

移除全部元素。

清空节点并回收存储，单调游标序号继续保留。

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

**源码**：[source/std/linked_list.lep:138](../../source/std/linked_list.lep#L138)。
