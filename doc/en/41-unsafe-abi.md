# Raw storage and C ABI directives

<!-- LANGUAGE NAVIGATION -->
**English** · [简体中文](../41-unsafe-abi.md)
<!-- END LANGUAGE NAVIGATION -->

## Unsafe scopes and storage

`inline unsafe T f(...)` declares module visibility and an unsafe calling contract. Put unsafe operations in the implementation inside explicit `unsafe { ... }` blocks. Closures are checked independently. A macro block grants permission to its own template and to macros called by that template. Argument fragments keep the permissions of the call site.

```lepus
import std::memory;

void main() {
    RawStorage<String> slots = RawStorage<String>::allocate(8);
    unsafe {
        initAt<String>(slots.pointer(0), "hello");
        String value = takeAt<String>(slots.pointer(0));
        assert(value == "hello");
    }
    slots.close();
}
```

`RawStorage<T>` owns uninitialized capacity and implements `Closeable`. Spare slots do not construct elements. `RawPtr<T>` holds allocation identity and an element index. The standard library encapsulates these fields. Use `RawStorage<T>::allocate` and `slots.pointer(index)` to obtain storage and slot pointers. A pointer at `capacity` is a one-past position and cannot be dereferenced. `__rawSize(T)` and `__rawAlign(T)` query language storage layout inside unsafe scopes. Direct array types and host aliases are unsupported raw elements. Wrap arrays in a named struct.

The public `std::core::rawSize<T>()` queries the same value layout from safe code. For example, `rawSize<uint8>() == 1` and `rawSize<int64>() == 8`. Aggregate sizes include their language storage slots. Owning values such as String report the value slot size, excluding separately allocated contents and capacity. This query does not access an object or describe the host C ABI `sizeof`. Void, direct arrays and host aliases are unsupported.

| Operation | Behavior |
| --- | --- |
| `initAt<T>(pointer, value)` | Requires an uninitialized slot; moves the supplied value into it |
| `readAt<T>(pointer)` | Reads a live slot using ordinary independent value semantics |
| `borrowAt<T>(pointer)` | Borrows a live slot; the caller must prevent use after cleanup, move-out or release |
| `takeAt<T>(pointer)` | Transfers ownership and marks the source uninitialized |
| `dropAt<T>(pointer)` | Cleans a live element while retaining capacity |
| `relocateNonOverlapping<T>(to, from, count)` | Validates the full ranges and states before moving; destination must be uninitialized and ranges disjoint; zero elements return immediately |
| `RawStorage<T>::close()` | Cleans only initialized elements, then frees storage; repeated close does nothing |

For statically identifiable slots, borrow analysis tracks references returned by `borrowAt` through their last use. It diagnoses intervening moves, drops, relocation and closure of the storage. For a dynamic index, `withBorrowAt(pointer, callback)` borrows during one synchronous callback. The runtime rejects conflicting operations and ends the borrow when the callback finishes.

Type identity, capacity overflow, alignment, initialization state and allocation lifetime checks remain. Supported raw alignment is at most 16 bytes. Unsafe operations impose extra obligations on the caller; these interfaces are not a safe reference API that bypasses ownership rules.

### Type-erased bare pointers and byte operations

`BarePtr<T>` encapsulates a Lepus logical byte address. When derived from raw storage, the pointer keeps the allocation identity through arithmetic and view conversions. `T` determines the static layout and arithmetic stride. `p + n`, `n + p` and `p - n` offset by `n * rawSize<T>()`. `p - q` requires one shared live allocation and returns an integral element distance. Dereferences and byte operations require `unsafe` and validate the original allocation and access range for derived pointers.

```lepus
unsafe {
    BarePtr<int64> p = barePointer<int64>(slots.pointer(0));
    BarePtr<int64> q = p + 1;
    assert(q - p == 1);
    memset<int64>(p, (uint8)0, 8);
    int64 mut& value = dereference<int64>(p);
    *value = 42;
}
```

Here `slots` is a `RawStorage<int64>`. `barePointer<T>` obtains an address, allowing uninitialized and one-past positions without checking the actual slot type. `pointerFromAddress<T>` forms a pointer from a numeric address, and `dereference<T>` interprets it with the static type. These operations require `unsafe`. Logical addresses cannot be passed directly as machine C pointers.

Lengths in `memcpy<T,U>(destination, source, bytes)` and `memset<T>(destination, uint8 value, bytes)` are byte counts. These operations check no element types, change no raw-storage initialization bits and invoke no copy, move or cleanup hooks. Nonempty ranges must lie within live allocations. The two complete copy ranges are validated before writing. Overlapping copies are rejected. Zero lengths allow empty pointers. A derived pointer remains invalid after release, including when a later allocation reuses the address. With `pointerFromAddress`, callers are responsible for the address's origin.

These types and operations belong to the implicit `std::core` module. `memcpy` and `memset` use LLVM memory intrinsics after runtime validation. The scalar kernel validates raw storage and the origin of derived bare pointers.


Experimental implementations live in `tests/fixtures/unsafe_storage/raw_containers.lep` and `tests/fixtures/unsafe_storage/bootstrap_string.lep`. List deletion cleans elements and retains capacity; Map owns key/value/state slots, and Set owns an independent key array. The String experiment stores UTF-8 bytes and implements capacity, append, byte access, naive search and explicit independent copying. It does not implement the full standard String Unicode API. Resource-owning types do not implement `Copy<T>`.

## Pointer views and IEEE754 bit patterns

Within an `unsafe` scope, `BarePtr<T>.castTo<U>()` returns a `BarePtr<U>` at the same address. It changes the element view without numerical conversion, allocation, or a target-type check. The caller must ensure the required range, alignment, initialization, and lifetime. Cast to `uint8` for byte-wise arithmetic and dereferencing; pointer offsets use the resulting element layout.

IEEE754 codecs use this method to reinterpret and dereference storage rather than obtaining bit patterns through numerical integer/float casts. Both `float32` and `float64` occupy eight-byte floating-point slots in the language. The four-byte external binary32 representation is therefore reconstructed from the slot's binary64 bit pattern, rather than reading its first four bytes as binary32. The LLVM backend lowers the supported conversions directly to LLVM `bitcast` and floating-point width conversions, and inlines pointer-view wrappers. Codec arguments explicitly select byte order.

## Default self-hosted standard library

Default List, Map and Set growth, lookup, insertion, deletion, copying and clearing live in `source/std/core.lep`. List removal and clearing retain capacity. Map combines dense key/value entries, occupied-slot encodings and cached HashKey values; Set owns independent slots. Existing `Equal`/`Hash` contracts and opaque public storage representations preserve host interoperability.

A compact numeric List can read its length through an LLVM view guarded by a revision and clear its occupied slots in one operation after validating the borrow state. Map and Set read the candidate slot and its cached hash together, then compare keys with matching hashes. Short String patterns use a first-byte scan followed by candidate comparison. The 64-bit native fast path can scan a validated span in bulk. Longer patterns use KMP.

Native optimization can store a borrowed interface's logical receiver address and concrete type tag in a compact slot. The object store shares the type name and formatting layout for the same binding. Dispatch validates the complete handle, and general object operations materialize the regular representation on demand. The concrete type's copy and drop protocols manage owned interface values.

Linked lists, stacks, queues, deques, ring buffers, binary search trees, red-black trees, general trees, min-heaps, graphs and union-find use Lepus implementations in their standard modules. List traversal, function-value callbacks, folds, binary search, extremes and stable index-merge sorting also run in the language. Algorithm leases retain the source and prevent structural movement during callbacks. String ownership, search, comparison, classification, UTF-8 decoding, slicing, transformations, splitting, joining and byte conversion, together with IEEE754 codecs and KMP, are implemented in Lepus.

Remaining host entry points cover these boundaries rather than the migrated container and text traversal algorithms:

- Allocation, release, address/length layout access, liveness and range checks, and raw-storage initialization state.
- Opaque single-slot reservation, swapping, reads, owned-value marshalling, key comparison/Hash callbacks and iterator leases, preserving existing host Codecs and record ABI.
- I/O, numeric display formatting, operating-system services, reflection, synchronous callbacks and C ABI. Big-number and CAS support retain their existing runtimes.

Existing resource-copy restrictions on safe containers remain in force. Experimental raw-storage containers separately verify moves and exactly-once cleanup of `Closeable` elements. The experimental byte string is retained as a separate functional fixture.

## Imports and exports

Ordinary `#abi` declarations can be called directly. Use an explicit `unsafe { ... }` block when the declaration is marked `unsafe` or its signature contains a bare pointer. The bridge marshals each level of a multilevel pointer. It stages one object from language storage for the duration of the call and writes changes back for writable views. Native addresses returned by the host can be compared with `null` and passed back through the C ABI. The bridge keeps native addresses distinct from language logical addresses.

```lepus
#abi("C", "./native.dll", "calc")
int32 nativeCalc(int32 x);

#export("calc")
int32 calc(int32 x) { return (int32)(x + (int32)1); }
```

Imports are declarations without bodies. The arguments specify calling convention, library path and native symbol; currently the convention is `"C"`. `#export("symbol")` selects the native symbol; `lepus build FILE --shared --out OUTPUT.dll` selects the output library. Export names must be valid, unique C identifiers outside runtime-reserved prefixes.

Imports retain synchronous callback and return-ownership adapters; release functions and array-length contracts are covered in [Host integration](08-host-integration.md). Imported function values use adapters with the language calling convention. The bridge performs the actual C call, without casting a runtime function value to a machine function pointer.

`#export` supports scalars and resource-free value structs. Generated C headers describe the platform C ABI, and the C++ compiler adapts by-value struct arguments and returns. String, containers, closures, references and structs requiring cleanup cannot cross this value-only export boundary directly. Hosts use the generated header and follow the `lepus_init`/`lepus_finish` lifecycle.

## Demand-driven native linking

The LLVM backend selects standard-library host modules from reachable native calls. The linker then collects unused function sections within selected modules. An unused source import does not force all its host code into the executable. Exports, reflection, callbacks and initialization/cleanup retain their reachability roots.

The external LLVM path defaults to O3 and full LTO, eliminating unreachable LLVM definitions during linking before section collection and symbol stripping. The static LLVM path is configured for O3, global dead-code elimination and LTO; release reports identify the platforms actually validated. `LEPUS_BUILD_FULL_STDLIB=1` forces selection of every standard host module for same-version comparisons.

On MinGW, the CAS 64 MiB zero-initialized backing pool occupies BSS instead of embedding the entire zero buffer in the file. Runtime pool capacity remains unchanged. File size and runtime memory use are separate measurements.

The native data image retains only data referenced by `addr` in reachable functions and records needed by function values, removing unreachable initializers and layout entries. Startup zeroes the logical image and copies nonzero segments; identical payloads share file storage. Binary layouts use an explicit byte length, supporting empty tables and embedded zero bytes. Logical addresses, object identity, array length headers and the heap boundary remain unchanged, so a smaller file does not imply a smaller runtime data area. `LEPUS_BUILD_KEEP_DATA=1` retains the complete image and layout for debugging comparisons.

## C frames for host entries

The standard library's direct C entries use `source/abi/lepus_abi.h`, a separate boundary from external signatures declared with `#abi`. A `LepusAbiFrame` contains 64-bit argument slots and a separate kind array. Integer slots also carry checked language handles; floating slots hold double bit patterns. A handle is not a dereferenceable host pointer.

A module calls `lepus_abi_local_info(requiredCapabilities)` using its own compiler, and the host checks that descriptor with `lepus_abi_check`. It records the ABI version, target architecture and platform, pointer width, byte order, C calling convention, and frame/record sizes, alignments and field offsets. Different versions, architectures/platforms, target layouts, packing or unsupported required capabilities are rejected. Copying the host descriptor does not describe a module's actual layout. Compatibility does not establish native execution support for a cross target.

`lepus_abi_check_module` validates a `LepusAbiModuleDescriptor` before registration, including missing names/functions, invalid arities and duplicate names. It does not register anything. `lepus_abi_check_frame` validates the argument count, arrays and slot kinds before a call; `lepus_abi_check_result` checks the returned kind. The native direct-entry path performs these checks as well. Callers must still supply accessible, sufficiently large memory: these functions cannot prove arbitrary pointers are readable.

`LepusAbiError` uses a caller-owned buffer without allocating or transferring ownership. `status` holds a stable error code; `required` includes the terminating zero byte, or is zero on success. Truncated output remains terminated, and zero capacity supports a length query. Protocol diagnostics use fixed text independent of locale. Run `python tests/lepus_c_api_contract.py` for independently compiled C11/C++17 interoperability and rejection of incompatible packing.

A language failure inside a synchronous C callback returns to the checked invoker. Its trampoline stores the failure and returns a zero value to C. After the C function returns, the adapter releases its owned return pointer before reporting the original failure; a C++ exception does not escape through the user's C function. Callback pointers are valid only for that synchronous call and cannot be retained for asynchronous invocation. A panic outside a callback still terminates the process.

## Reproducing experiments

```text
python tests/lepus_unsafe_storage.py --aot
python tests/lepus_abi_directives.py
```


## Current primitive lowering

| Operation | LLVM path | Types and retained rules |
| --- | --- | --- |
| `memcpy` / `memset` | `llvm.memcpy` / `llvm.memset` after range preparation | Byte counts; validate complete ranges before writing, reject overlapping copies, permit null for zero length. |
| Logical addresses and bare-pointer arithmetic | Address values and integer arithmetic, with small wrappers inlined by O3 | Element strides use language layout; logical addresses are not machine pointers. |
| Scalar raw-slot access | LLVM loads/stores or C-signature scalar primitives for supported layouts | bool, char, signed/unsigned integer widths and float32/64; preserve narrowing, signs, negative zero and floating-point bits. |
| `rawSize<T>()` | Language-layout constant in each generic instance | One byte for uint8/char, eight-byte ordinary value slots; aggregate language layout excludes dynamic allocations. |
| `initAt` / `readAt` / `takeAt` / `dropAt` | Fixed semantic lowering plus initialization-state and cleanup kernels | Owned values follow their type protocols; bit copies do not establish ownership. |
| C entry results | Return checked C-frame bits and result kind directly | Errors, memory faults, step limits and exit requests are checked first; scalar results avoid generic-value conversion. |

Complex layouts, object lookup, resource protocols, callbacks and stale-access diagnostics retain their C runtime services. Checked and unchecked keep their documented array/pointer settings; static borrowing and ownership cleanup remain in force. The C++ backend generates production LLVM IR. Python tests invoke the compiler and verify results, IR and target files.


## Directly linked C ABI

Use `@link` for fixed scalar C signatures supplied by a linked object or archive.

The compiler preserves scalar widths and uses the target C calling convention. Use a compatible dynamic library for structures, references, buffers and synchronous callbacks. The existing marshalling rules above describe those signatures and ownership.

Run `python tests/lepus_link_abi.py --lepus PATH` to check scalar calls, target declarations, rejection of complex direct-link signatures and exports to a C host.
