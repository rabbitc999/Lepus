# Reflection serialization and dynamic SQL

<!-- LANGUAGE NAVIGATION -->
**English** · [简体中文](../33-reflection-serialization-sql.md)
<!-- END LANGUAGE NAVIGATION -->

`std::json`, `std::toml`, and `std::serialization` generate typed codecs from reflection
metadata. Expose user structs and enums with `@Reflect`. Field access follows ordinary
copy, destruction, and borrowing rules; codecs do not reinterpret runtime addresses.

Annotated SQL models still use the reflection switch. Codec models can use member macros with `--no-reflection`.
The generic codecs accept models registered by the JSON macro or exposed with `@Reflect`.

## Codecs and fields

Member macros generate typed encode and decode methods with caller-selected names.
JSON uses `$std::json::Json<T>(toJson, fromJson);`, TOML uses
`$std::toml::Toml<T>(toToml, fromToml);`, and the byte codec uses
`$std::serialization::Binary<T>(toBinary, fromBinary);`. The generated methods and
generic functions share the same codecs and data formats. The JSON macro registers
the model for all three member macros, which also work with `--no-reflection`. Existing `@Reflect` models can continue
to use the generic functions.

Each module provides `serialize<T>(T const& value)` and `deserialize<T>(...)`, returning
`Result<Output, Error>`. JSON and TOML use text; serialization uses `List<uint8>`.
Pass `&value` when encoding. Decoded values own independent storage.

Codecs validate missing fields, numeric ranges, input kinds, and supported types.
Integers never pass through floating point. Large `uint64` values use decimal strings;
`BigInteger` uses decimal text and `BigDecimal` stores both decimal text and precision.
Zero precision keeps the default; explicit precision supports 1–100000 digits. Decimal
exponents have an absolute limit of one million, matching the numeric backend.
NaN and infinity are rejected. Enums store `$variant` and `$values`. Lists, sets,
string-keyed maps, and nested reflected types are recursive, with a depth limit of 128.
Owning resources, functions, pointers, and unsupported composite types return errors.

TOML requires an object root. Reflection encoding creates a new document; preserving
comments from an existing TOML document remains the responsibility of `parse(...).dump()`.
The reflection switch leaves the byte format unchanged. It stores `MGS`, a version, a little-endian 64-bit type identifier, a
little-endian 64-bit payload length, and UTF-8 JSON. Decoding validates all header fields
and the exact length. It does not serialize native memory layouts.

Use `std::reflect::getStructInfo<T>()` to query field names, type IDs, layout, and
annotations. Use ordinary member access to read or write values. Compile-time
`$fields<T>()` can generate code that visits the fields.

## Dynamic SQL

`std::sql` expands model and field annotations at compile time to generate the ipage model description. `Query` takes filters, sorting and pagination at runtime.
The maintained source is `library/ipage/source/ipage`; exported projects use embedded assets.
Declare `@Reflect` and `@std::sql::ModelClass("table")` on the model, and optionally
`@std::sql::ModelField(column: "column", kind: "auto", id: true)` on fields.
Kinds are `auto`, `int`, `real`, `text`, and `bool`; every model needs a primary key.

`std::sql::query<T>()` creates a value-semantic query. Configure it with `std::sql::filter<V>`,
`std::sql::filterMany<V>`, `orderBy`, `select`, and `page`. `build()` returns a `SqlStatement`
containing `sql` and `parameters`, a JSON scalar array. Use `build("create")` for a table,
`build("count")` for counting, `std::sql::insert<T>` or `std::sql::update<T>` for model writes, and
`build("delete")` for deletion. Updates and deletes require filters.

Operator numbers follow ipage: 0 equal, 1 unequal, 2 greater, 3 less, 4 greater-or-equal,
5 less-or-equal, 6 contains, 7 starts-with, 8 ends-with, 9 null, 10 not-null, 11 IN,
12 NOT IN. Use `filterMany` for multi-value and null conditions. Column names are
validated against model metadata and quoted by the dialect. Values only enter the
parameter list and placeholders.

`Database.exec(statement)` executes a bound statement directly. Dynamic generation and
direct parameter binding currently support SQLite. Binding preserves integer, float,
text, bool, and NULL types, checks parameter counts, and accepts one statement only.
Other existing database backends retain text execution and explicitly reject bound
execution. They never interpolate parameter values as a fallback.

Runnable examples: [fields](../../example/samples/reflect_fields.lep),
[serialization](../../example/samples/reflect_serialization.lep), and
[SQL](../../example/samples/sql_ipage.lep).

The ipage copy contains only the C++17 SQL generation core. SQLite dialect and execution adapters belong to Lepus std::sql glue.
