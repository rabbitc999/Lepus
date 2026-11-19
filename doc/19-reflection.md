# 注解与反射

<!-- LANGUAGE NAVIGATION -->
[English（中文原文）](19-reflection.md) · **简体中文**
<!-- END LANGUAGE NAVIGATION -->

反射针对编译期已存在、显式暴露的具体声明，提供元数据与类型化查询。
编译器生成普通 Lepus 桥接函数，运行时复用精确类型检查、资源清理和实际接口分派。
值语义、非空引用、确定性 Drop 和借用检查继续适用；分配使用现有 Arena，不承诺零分配或硬实时。

## 注解语法

注解声明仍用规格中的名称在前、类型在后的参数形式；被注解的函数、字段和构造函数遵循 Lepus 现行 C 风格语法：

```mc
@annotation Component
@annotation http::Route(path: String, level: int32 = 2)

@Component
struct Service {
    int64 value;
    @http::Route("/read", level: 3)
    int64 read() { return self.value; }
};
```

注解声明只允许在模块顶层，没有结尾分号。可以连续使用多个注解，参数可用位置或名称绑定，位置参数须在命名参数之前；重复赋值、未知名称、缺少必填项或多余实参报错。默认参数可以位于必填参数前，此时后面的必填项可用命名实参提供。紧随注解名的 `(` 总是注解参数；标注元组返回函数时用 `@Tag()` 明确结束注解。

注解可附着于 struct、enum、interface、自由函数（含宿主函数声明）、module，以及结构体/枚举的实例方法、静态方法、构造函数、字段和静态字段；成员本身必须符合现有语言规则（例如枚举不因此新增实例字段或构造函数）。局部变量、表达式、语句块、import、宿主类型声明和全局变量不可标注。模块标注通过生成解析器/`--raw` 输入可用；普通模块加载器仍要求文件头第一条有效语句为 `module`，因此普通项目入口不支持在 module 之前放注解。

名称必须先声明后使用，不允许覆盖。无显式限定的声明属于当前模块；使用先找当前模块、根模块，再找直接导入模块中的唯一候选。模块仍各用独立文件和现有 import 文件头规则。注解不能与同一命名空间的类型重名，后续类型声明也参加冲突检查。

注解参数支持 bool、char、定宽有符号/无符号整数、float32/float64、String（核心限定可省略为 String）；拒绝 void、枚举、结构体、接口、宿主别名、容器、引用及函数指针。默认值和实参均限字面量、数字字面量的一元正负号以及类型完全匹配的显式 cast。整数与浮点字面量按目标类型检查范围；`2` 不能用于浮点参数，`2.0` 不能用于整数参数，`(int64)2` 不能用于 int32 参数。沿用既有十进制词法上限，`int64` 最小值用 `-0x8000000000000000`，`uint64` 最大值用 `0xFFFFFFFFFFFFFFFF`。字符串/字符复用现有转义检查。变量、函数调用和 `1+2` 等表达式均拒绝。

前置注解的泛型模板尚不支持，会在语法检查时报错；普通泛型声明、单态化类型作为普通函数签名，以及被注解的具体声明可以共存。具体函数签名中已单态化的类型会登记规范身份；目前不能用前置 `@Reflect` 标记泛型模板，也没有独立的具体泛型类型暴露语法。

AST 保留每个声明的 `annotations` 节点列表、每次使用的声明绑定，以及按参数声明顺序补齐的 `annotationValues`。常量保存编译器拥有的值和精确类型身份，不保存运行时句柄或引用。`M_ParsedNode` 是编译器内的只读 AST 接口，不能作为 Lepus 反射查询接口使用；运行时元数据从这些 AST 绑定生成，执行语义与原声明一致。

语义错误指向注解起始 `@` 的文件、行、列；预处理包含文件映射同样有效。错误位置不是参数字符的单独定位，普通语义错误仍沿用原有整行范围。

## 默认配置与产物兼容

根目录工具链通过 `features.mk` 把两项钉死为 `1`。独立生成的编译器工程通过两个默认 `ON`
的 CMake 选项同时编入前端与运行时反射；将其关闭只用于能力兼容验证，不属于受支持的语言裁剪配置。

构建根目录工具链：

```sh
make
./lepus run source.lep --reflection --out tmp/program
```

源码预处理的 `-D` 不能替代工具链构建宏。单次编译默认给产物写入反射要求；`--no-reflection`
不写这份元数据，并拒绝程序里的注解与反射使用 —— **它只决定这次的产物与这次程序能用什么，
不减少编译期工作量**：元数据与查询源码那一轮照跑。`--diagnostics=json` 使用既有诊断 schema。
检查模式仍然不生成产物。

`make` 已把两项钉死为 1；直接 `-D` 手编或调整独立生成工程的 CMake 选项可以把它们置 `0`。这些配置只用于验证编译器自身（下表），不是受支持的裁剪配置：

| 前端宏 | 运行期宏 | 普通编译 | `--reflection` |
| --- | --- | --- | --- |
| 0 | 0 | 允许 | 拒绝 |
| 0 | 1 | 允许 | 拒绝 |
| 1 | 0 | 允许 | 拒绝 |
| 1 | 1 | 允许 | 允许 |

前端查询实际链接的 `supportedProgramFeatures()`，不会仅凭自身头文件中的运行期宏判断。Make 构建配置指纹包含两个宏，切换后会重编译；单次 `--reflection` 进入增量产物缓存签名。独立生成的编译器工程提供两个同名 CMake 选项，均默认 `ON`。

## 产物兼容规则

`QuadFile` 与 `Program` 使用 `requiredFeatures` 保存程序要求的能力。当前定义 `0x0002` 为反射位；未知能力位一律拒绝。四元式通过 `# lepus-features: reflection` 保存要求，优化保留此字段。未知的 `lepus-features` 值报错，不能悄悄降级为普通程序。

四元式头 `MGQI` 的特征位里 `0x0002` 表示反射，`0x0001` 不是有效标志，LLVM 构建 后端会拒绝。没有开启反射的产物保持原有字段值，不添加反射数据，也不升级格式版本。

后端按能力包含关系校验：带反射支持构建的编译器能编译普通程序；关闭反射支持的手编编译器不能编译要求反射的程序。编译失败保留原四元式产物，拒绝发生在写文件之前。能力位是兼容契约，不是防篡改签名。

## 元数据模型与传输

C++ 宿主值模型与 Lepus 类型化查询使用同一元数据来源。`StructInfo`、`EnumInfo`、`InterfaceInfo`、`FuncInfo`、`MethodInfo`、`AnnotationInfo` 记录布局、签名和注解；`AnnotationInfo` 保存参数声明与可选默认值，`Annotation` 保存某次使用已按参数顺序补齐的常量值。枚举通过多个具名 `payloads` 保留 Lepus 原生语义。方法属性记录实例/静态、构造、只读接收者与接口默认体存在性，不含可调用地址。

身份由带类别前缀、完整模块名称及具体签名的规范键计算 FNV-1a 64 位编号，表校验拒绝重号或不一致的键。它独立于 Any 与接口的私有运行时分派编号；应用不能读取或持久化这些内部编号。规范键由 Lepus 编译器统一生成。查询按编号匹配，按名称查询遇到重载返回空；结果为 `std::optional<std::reference_wrapper<const T>>`，生命周期受表约束，调用者不能通过查询修改记录。

独立段以 `MCRF` 魔数和小端 32 位 schema=1 开始，随后按模型字段顺序编码。编号及数字位模式使用小端 64 位，长度与偏移为小端 32 位，布尔/可选标志仅接受 0/1；字符串保存原始字节，支持内嵌零。段最多 16 MiB，每个列表最多 4096 项，每个字符串最多 1 MiB，读取分配前检查剩余长度。解码后检查身份、类型引用、注解参数精确类型、字段重名和枚举标签重复。编解码失败保留原输出，加载失败保留 Program，保存拒绝保留已有文件。

四元式注释 `# lepus-reflection: <十六进制段>` 可携带表，须同时声明 `# lepus-features: reflection`，重复段或非法编码报错。优化保留表。发布构建把编码数据固化为 C++ 静态字节数组，并提供宿主拥有型表工厂 `lepus_reflection_metadata()`；每次通过同一 codec 在调用方 Memory 分配域解码，表由 Program 或宿主局部变量持有并按作用域释放，静态只保存编码字节，运行时不读元数据文件。元数据解码不构造被描述的 Lepus 对象。被注解的名称按原样保留在表里，反射本身就是显式暴露。

## 显式暴露声明

内建零参数标记 `@Reflect`（也可写 `@std::reflect::Reflect`）仅暴露有实现的自由函数，以及具体结构体、枚举和接口。成员随类型暴露，不支持单独标记字段或方法；未标记类型只在已暴露签名或接口关系需要时登记身份，不生成其完整记录。用户已声明的同名注解优先，保留已有代码含义；需要明确内建标记时用限定名称。关闭开关时仍在注解前端报错，完全不生成反射段。

```mc
@annotation Route(path:String)
@Reflect
struct Service {
    int64 value;
    Service(int64 value) { self.value = value; }
    @Route("/read")
    int64 read() { return self.value; }
    static int64 answer() { return 42; }
};
@Reflect
int64 addOne(int64 value) { return value + 1; }
```

生成发生在所有实例化、名称解析、布局和方法降级完成之后，检查模式也校验表，但不写产物。类型身份保留模块、位宽、符号、引用权限、容器和具体泛型实参；函数/方法身份同时包含归属和完整具体签名，不改变内部编号。字段偏移使用编译器既有聚合存储布局的八字节槽位，不是宿主手写 C++ 类型的 `offsetof`。拥有标志复用既有资源分类；此模块仅描述布局，不实现反射字段写入。

类型的实例方法、静态方法和构造函数均进入表，显式参数不包含 self。构造函数显示为 `new`，记录所属类型为返回类型；方法只读性复用现有编译器方法体分析。接口表保留默认体存在性，具体类型表保留已解析的默认方法和原注解；接口双向关系含继承关系。

注解定义独立保留参数和默认值；已暴露声明及成员保留绑定后的实例参数。内建标记使用独立身份域，避免与用户同名注解碰撞。schema=1 的模块注解列表仍是汇总列表，不额外保存所属模块。示例见 `example/reflection/declarations.lep`。

## Lepus 类型化查询

在默认启用反射的工具链中使用 `import std::reflect;`；`--reflection` 可显式要求能力。`getStructInfo<T>`、`getEnumInfo<T>`、`getInterfaceInfo<I>`、`getFuncInfo(name)` 和 `getAnnotationInfo(name)` 返回 `Option`，缺失或名称不能唯一选择重载时返回 `None`。具体类型的 `getTypeId<T>` 返回稳定规范身份，`getInterfaceId<I>` 使用同一身份机制；`implementsInterface<T>(id)` 查询已暴露类型的接口关系。

`getAnnotations<T>`、`getFieldAnnotations<T>(name)`、`getFuncAnnotations(name)`、`getMethodAnnotations<T>(name)` 和 `getStaticMemberAnnotations<T>(name)` 返回注解实例列表，缺失为空列表。`AnnotationInfo` 保存定义，`Annotation` 保存定义身份和按声明顺序补齐的实参；默认值与实参使用封闭 `ReflectValue`，其 `asI64/asText` 等读取方法返回精确类型 `Result`。整数位宽和符号必须精确匹配。

编译器在完整布局和单态化后生成普通 Lepus 查询函数，再通过权威文法解析和正常降低。三条后端共用值复制与 Drop 规则；查询返回独立拥有型快照，修改快照不改变编译时表。内部编号不作为稳定公开身份。没有用户声明时小写 `string` 在类型位置报普通未知类型错误；字符串规范身份为 `String`。

## 从动态反射迁移

使用直接函数调用、普通接口分派或具体类型的构造入口。应用需要按字符串分发时，在入口显式维护名称到行为的映射。通过普通成员访问读写对象值，或用 `$fields<T>()` 在编译期生成逐字段代码。`getStructInfo<T>()` 仍可读取字段名称、类型与布局信息。

`ReflectValue` 继续承载注解实参和默认值；其 `asXxx()` 接口在类型不匹配时返回 `ReflectError::TypeMismatch`。
`--hard-realtime` 与 `--reflection` 同时出现时拒绝编译。

## 示例与验证

以下回归分别检查能力矩阵、注解、元数据、查询、旧入口的编译期拒绝及端到端行为。
根目录工具链保持前端和运行时反射同时开启，关闭能力配置仅用于交叉加载与裁剪验证。

```sh
python tests/lepus_reflection_features.py
python tests/lepus_annotations.py
python tests/lepus_reflection_metadata.py
python tests/lepus_reflection_declarations.py
python tests/lepus_reflection_queries.py
python tests/lepus_reflection_calls.py
python tests/lepus_reflection_e2e.py
```

[声明示例](../example/reflection/declarations.lep)展示标记和元数据；
[JSON 示例](../example/reflection/json.lep)用已知字段构造模型，仍不是通用反序列化器；
[接口示例](../example/reflection/dispatch.lep)检查普通接口分派和默认实现。
查询得到的拥有值遵守普通复制和清理规则，修改查询快照不会修改编译时表。
