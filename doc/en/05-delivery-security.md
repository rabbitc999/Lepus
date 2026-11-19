# 05 Delivery security

<!-- LANGUAGE NAVIGATION -->
**English** · [简体中文](../05-delivery-security.md)
<!-- END LANGUAGE NAVIGATION -->

Native executables are never encrypted or obfuscated: `--encrypt` and `[lepus] encrypt` are unsupported, so deliverables always come from source through native compilation.

## Delivery forms

| Artifact | Recipient needs | Typical use |
| --- | --- | --- |
| `.lep` source | Lepus compiler | Source sharing and joint development |
| `build` executable | Target platform; any dynamic library the program uses must sit next to it | Closed-source delivery and offline runs |
| `--shared --export` library + header | A C/C++ host project and linker | Embedding in an existing host |
| LLVM IR (`build --emit-llvm`) | A toolchain able to assemble it | Inspecting the intermediate form, custom assembly |

## Export and distribute

Executable:

```sh
lepus build src/main.lep --out build/app
```

Library plus header:

```sh
lepus build src/lib.lep --shared --export add --out build/lib
```

The second command produces `build/lib.dll` (`.so` on Linux) and `build/lib.h`; the host calls `lepus_init()`, then `f_add(...)`, then `lepus_finish()`. On Windows the loader searches the executable's directory and `PATH`, so libraries referenced by `#abi` must be placed next to the artifact.

To inspect the intermediate form only, run `lepus build src/main.lep --emit-llvm --out build/app`. It generates `.ll` directly from the AST without generating quadruples, linking, or executing.

## Boundaries

Executables carry no publisher authentication. Release provenance and access control belong to the deployment system, code signing or the package manager.

Default builds drop symbols and debug information (`--strip-all`). Pass `--debug` to keep function names and source locations; that mode compiles at `O0` and retains debug sections. `--debug-optimized` retains DWARF at `O3`; see [native debugging](43-native-debugging.md). String literals, type names and the [reflection table](../19-reflection.md) still appear in the artifact — if that matters, do not ship the source.

## Artifact management

| File | Kept by | Purpose |
| --- | --- | --- |
| `.lep` | Developer or collaborators | Source and rebuilds |
| Executable / library + header | Runtime recipient | Running directly or embedding |
| `.lepus/` intermediates | Build machine | Removed after a default build; for troubleshooting only |
| Debug builds and build records | Developer | Reproduction and troubleshooting |

Rebuild from source when upgrading, and recheck target architecture, dynamic library dependencies and system services. Legacy project keys such as `lepus.encrypt` now fail loudly instead of being ignored.

## Next

- [Toolchain](09-toolchain.md): commands and builds.
- [Build artifacts and intermediates](../15-artifacts.md): layout, the `.lepus/` directory and cleanup rules.
- [Host integration and the C ABI](08-host-integration.md): calling conventions in both directions.
- [Panic traces](../18-panic-trace.md): diagnostics and location mapping.
