# 01 Five-minute quickstart

<!-- LANGUAGE NAVIGATION -->
**English** · [简体中文](../01-quickstart.md)
<!-- END LANGUAGE NAVIGATION -->

Goal: run your first Lepus program within five minutes. You do not need to understand value semantics
and references first — that is [Core concepts](03-core-concepts.md).

---

## 1. Install

Two ways; pick one:

**Option 1: use a release package**

**Option 2: build from source**

```sh
sh gen.sh      # generate the front end from lepus.gram with IELR (needs the HareParse generator)
make           # build lepus and build/lepus-frontend/libparser.a
```

`gen.sh` needs a [HareParse](https://github.com/rabbitc999/HareParse) grammar generator.
The easiest route is to drop it in the project root as `./HareParse` (add `.exe` on Windows);
alternatively leave it in the neighbouring HareParse repository and `gen.sh` will find
`../HareParse/build/bin/` automatically.

On Windows, if no generator is found, `gen.sh` tries the latest release on [GitHub](https://github.com/rabbitc999/HareParse/releases/download/latest/HareParse.exe), and keeps whichever succeeds first as `HareParse.exe` in the project root. Existing generators are reused. Other platforms require a native executable, optionally selected through `ENGINE`.

Verify the install:

```sh
lepus help
```

> On Windows the binary is `Lepus.exe`. Under Git Bash / MSYS write `./lepus`, which resolves to
> `Lepus.exe`; a bare `lepus` goes through PATH lookup.

---

## 2. The first program

Create `hello.lep`:

```lepus
void main() {
    println("Hello, Lepus!");
}
```

Three things worth noting:

- The entry function is always `main`, returning `void`. Normal completion of `void main` exits with code 0.
- `println` comes from the automatically loaded core library; its full name is `println`, and
  the prefix can be dropped when there is no name clash.
- Semicolons and braces work as in C-family languages — deliberately so. Lepus borrows C's expression
  and block syntax.

---

## 3. Run it

```sh
lepus run hello.lep
```

Output:

```text
Hello, Lepus!
```

Handing a source file straight to `lepus` also compiles it with checked native compilation and runs it, and everything after it is passed to
`main` as program arguments:

```sh
lepus hello.lep Alice "two words"
```

When you need arguments, write the entry as:

```lepus
void main(List<String> arguments) {
    println("hello, {}", arguments[0]);
}
```

---

## 4. What happened

`lepus run` performs these four steps and launches the generated native program after successful compilation:

1. **Load source** — load `import` modules, retain source locations, and expand interpolation and macros;
2. **Parse and type-check** — syntax, types and reference lifetimes are checked here; nothing runs if
   this fails;
3. **Generate LLVM IR** — the normal execution path lowers the checked AST directly to LLVM IR;
   Lepus quadruples use a separate export path;
4. **Compile and run** — LLVM produces a native program with runtime checks, publishes it beside the source, reports compilation completion and launches the program.

To stop after the first two steps (no execution, no output files):

```sh
lepus check hello.lep
```

---

## 5. Next

| You want | Where |
| --- | --- |
| Learn the language features | [Language tour](02-language-tour.md) |
| Deploy the program to a target host | [Delivery security](05-delivery-security.md) → [Cross-platform deployment](06-deployment.md) |
| See what the standard library offers | [Standard library overview](07-stdlib-overview.md) |
| Start a real project | `lepus new my-app`; configuration in [Toolchain](09-toolchain.md) |

---

## 6. Command cheat-sheet

| Task | Command |
| --- | --- |
| Compile and run | `lepus run src/main.lep` |
| Check only, do not run | `lepus check src/main.lep` |
| JSON diagnostics (for editors) | `lepus check src/main.lep json` |
| Export an executable | `lepus build src/main.lep --out build/app` |
| Export LLVM IR | `lepus build src/main.lep --emit-llvm --out build/app` |
| Create a project | `lepus new hello` |
| Run tests | `lepus test tests` |
| Format check / write | `lepus fmt check src` / `lepus fmt write src` |
| Show help | `lepus help` |

More subcommands, `lepus.toml` configuration and compile options: [Toolchain](09-toolchain.md).

> Reading the English edition? Snippets are identical to the Chinese edition and are executed before
> publication. See the [Chinese edition](../01-quickstart.md).

Generated parser sources, headers and reports live in `source/`. Object files and the parser archive live in `build/lepus-frontend/`, keeping the repository root clear.
