"""验收已删除的动态反射操作在编译期被拒绝。"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import os
from pathlib import Path
import subprocess

from compiler_path import compiler_path
from project_temp import tempfile


PREFIX = """import std::reflect;
@Reflect struct Record { int64 value; };
@Reflect enum State { Ready };
@Reflect interface View { int64 read(); };
void probe(View mut& view) {
    Record record;
    State state = State::Ready;
    List<std::reflect::ReflectValue> args;
"""
CASES = {
    "call": '("program::probe", args)',
    "callMethod": '<Record>(&record, "read", args)',
    "callMethodConst": '<Record>(&record, "read", args)',
    "callStatic": '<Record>("read", args)',
    "callEnumMethod": '<State>(&state, "read", args)',
    "callEnumMethodConst": '<State>(&state, "read", args)',
    "callEnumStatic": '<State>("read", args)',
    "construct": '<Record>(args)',
    "callInterface": '<View>(view, "read", args)',
    "callInterfaceConst": '<View>(view, "read", args)',
    "getField": '<Record,int64>(&record, "value")',
    "setField": '<Record,int64>(&record, "value", 7)',
}


def main():
    """对照可编译输入，逐个检查已删除的入口。"""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--lepus", type=Path, default=compiler_path())
    compiler = parser.parse_args().lepus.resolve()

    with tempfile.TemporaryDirectory(prefix="reflection-removed-") as directory:
        work = Path(directory)

        def check(name, expression):
            folder = work / name
            folder.mkdir()
            source = folder / "probe.lep"
            source.write_text(PREFIX + expression + "; return; }\nvoid main() {}\n", encoding="utf-8")
            return subprocess.run(
                [str(compiler), "check", str(source), "--reflection", "--lang", "en"],
                env=dict(os.environ, LEPUS_LANG="en"),
                capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=240,
            )

        control = check("control", "let info = std::reflect::getStructInfo<Record>()")
        assert control.returncode == 0, (control.stdout, control.stderr)

        def rejected(case):
            name, arguments = case
            result = check(name, "std::reflect::" + name + arguments)
            assert result.returncode != 0 and name in result.stderr, (name, result.stdout, result.stderr)

        with ThreadPoolExecutor(max_workers=4) as pool:
            list(pool.map(rejected, CASES.items()))
    print(f"已确认 {len(CASES)} 个动态反射入口在编译期被拒绝。")


if __name__ == "__main__":
    main()
