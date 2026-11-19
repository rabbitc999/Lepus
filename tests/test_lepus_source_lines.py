"""验证 LEPUS 原始源码、宏、导入模块、子模块与导出的行级调试信息。"""
from compiler_path import compiler_path
import argparse
from pathlib import Path
import subprocess
from project_temp import tempfile
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--lepus', default=compiler_path())
    args = parser.parse_args()
    compiler = Path(args.lepus).resolve()
    with tempfile.TemporaryDirectory(prefix='lepus-lines-', ignore_cleanup_errors=True) as folder:
        root = Path(folder)
        source = root / 'main.lep'

        def run(text, expected, extra=()):
            source.write_text(text, encoding='utf-8')
            result = subprocess.run([str(compiler), 'run', str(source), '--out', str(root / 'out'), *extra], capture_output=True, timeout=20)
            error = result.stderr.decode('utf-8', errors='replace')
            assert result.returncode != 0 and 'panic' in error, error
            assert expected in error, error
            return error
        run('void main() {\n std::core::panic("plain");\n return;\n}\n', 'main.lep:2')
        source.write_text('void main() {\n int64 zero=0;\n let __boom = 1/zero;\n}\n', encoding='utf-8')
        raw = subprocess.run([str(compiler), 'run', str(source), '--raw', '--out', str(root / 'raw')], capture_output=True, timeout=20)
        assert raw.returncode != 0 and 'main.lep:3' in raw.stderr.decode('utf-8', errors='replace'), raw.stderr
        (root / 'included.lep').write_text('module included;\nint64 included() {\n std::core::panic("include");\n return 0;\n}\n', encoding='utf-8')
        run('import included;\nvoid main() { let __boom = included::included(); }\n', 'included.lep:3')
        (root / 'worker.lep').write_text('module worker;\nint64 crash() {\n std::core::panic("module");\n return 0;\n}\n', encoding='utf-8')
        run('import worker;\nvoid main() { let __boom = worker::crash(); }\n', 'worker.lep:3')
        macro = '$macro CRASH(x) -> {\n std::core::panic(x);\n}\nvoid main() {\n $CRASH("macro");\n return;\n}\n'
        run(macro, 'main.lep:5')
        run('$macro INNER(x) -> {\n std::core::panic(x);\n}\n$macro OUTER(x) -> {\n $INNER(x);\n}\nvoid main() {\n $OUTER("nested");\n return;\n}\n', 'main.lep:8')
        run('$macro STEP(x) -> {\n int64 temporary = x;\n}\nvoid main() {\n $STEP(1);\n std::core::panic("after");\n return;\n}\n', 'main.lep:6')
        run('$macro MANY(...values) -> {\n$for (item in values) {\n std::core::panic(item);\n}\n}\nvoid main() {\n $MANY("loop");\n return;\n}\n', 'main.lep:7')
        run('$macro JOIN(left, right) -> {\n $concat(left, right)\n\n\n}\nint64 joined_name=1;\nvoid main() {\n int64 value=$JOIN(joined_, name);\n std::core::panic("paste");\n let __boom = value;\n}\n', 'main.lep:9')
        run('$macro COMMENT(x) -> {\n /* don\'t "\n comment */\n std::core::panic(x);\n}\nvoid main() {\n $COMMENT("comment");\n return;\n}\n', 'main.lep:7')
        run('void main() {\n std::core::String text="hello\nworld";\n std::core::panic("multiline");\n return;\n}\n', 'main.lep:4')
        print('LEPUS original lines + imported module + macro + privacy' + ': PASS')
if __name__ == '__main__':
    main()
