# 编译期用户注解样例

<!-- LANGUAGE NAVIGATION -->
[English（中文原文）](README.md) · **简体中文**
<!-- END LANGUAGE NAVIGATION -->

`annotations.lep` 展示注解声明、默认值、位置与命名实参，以及接口、结构体、枚举、字段、静态字段、构造函数和方法上的注解。函数与成员使用 Lepus 现行 C 风格语法。

先按 [注解与反射文档](../../doc/19-reflection.md) 开启编译器和 VM 的两项构建宏，再从仓库根执行：

```sh
lepus/lepus --reflection lepus/example/reflection/annotations.lep --out build/annotation-example
```

预期输出：

```text
annotations 42 7
```

`@Reflect` 将显式暴露的声明加入元数据表。`queries.lep` 展示类型化查询，`dispatch.lep` 展示普通接口分派。

运行专项验证：

```sh
python tests/lepus_annotations.py
```
