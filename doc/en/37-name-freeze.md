# Lepus name and related capabilities

<!-- LANGUAGE NAVIGATION -->
**English** · [简体中文](../37-name-freeze.md)
<!-- END LANGUAGE NAVIGATION -->

This page records naming and capability references. Actual scope comes from implementation, reference documentation and verification, rather than unconditional guarantees for every platform, external library or unsafe operation.

Lepus is the scientific name of the hare genus and of the constellation Lepus. It is not an acronym; the language makes five explicit promises: value semantics, macro hygiene, type erasure, reference validity, and interface dispatch. The sections below describe the behavior behind each promise.

## Names and files

| Item | Current spelling |
| --- | --- |
| Language | Lepus |
| Source suffix | .lep |
| Diagnostic prefix | LEP |
| Project configuration | lepus.toml |
| Standard-library compound types | PascalCase: `DateTime`, `XmlNode`, `EventBus` |
| Standard-library methods and module functions | camelCase: `childCount`, `isPlaying`, `formatUtc` |
| Standard-library modules | snake_case: `std::date_time`, `std::number_theory`; single-word modules such as `std::gui`, `std::audio`, `std::xml` remain lowercase |
| Compiler artifacts | Version, LLVM linkage, system and architecture in the filename |

Commands normally use lepus; follow the [installation guide](39-installation-packages.md) for actual build filenames. Git and CHANGELOG retain historical names.

## Value semantics: copies and moves

Ordinary values retain independent observable value semantics; reference fields still express borrowing. Explicit moves transfer ownership. The compiler may remove copies or reuse storage while preserving behavior, so copy syntax does not imply a physical deep copy every time. Resource types retain noncopyable restrictions. See [values](../book/en/07-value-semantics.md) and [moves](24-ownership-move.md).

## Macro hygiene and evaluation

Internal names are isolated from callers. Default value arguments evaluate once and mutations affect their copies. Reference arguments and `expr` syntax parameters have their own evaluation rules. Item and member macros declare their result category. See [macros](../book/en/22-macros.md).

## Type erasure

Any retains concrete type information for checked recovery. Erasure does not cancel provenance, ownership or resource rules, or invent interface implementations. See [Any and generics](23-any-generics-data.md).

## Reference validity

Static checks track sources and use scopes. Programs must also meet external C ABI and unsafe preconditions: static borrowing does not validate arbitrary external binaries. See [references](../book/en/08-references.md) and [unsafe boundaries](41-unsafe-abi.md).

## Interfaces and dispatch

Interfaces describe behavior implemented by concrete types. They support generics, inheritance and concrete-type matching. Behavior follows the actual receiver; known targets may be devirtualized or inlined while preserving observable results. See [interfaces](../book/en/17-interfaces.md).

## Verification

[Successful samples](../../example/samples/) and [rejected samples](../../example/samples-error/) provide concrete cases. Report source, compiler, target and options with results. Names and individual examples do not establish untested platforms or configurations.
