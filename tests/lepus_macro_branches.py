"""验证生成解析器的五类完整片段、编译期分支及 ECMAScript 原文匹配。"""
from compiler_path import compiler_path
import argparse
import os
from pathlib import Path
import subprocess
from project_temp import tempfile
ROOT = Path(__file__).resolve().parents[1]


def main():
    """运行真实原生产物，检查分支顺序、未选中语义隔离、原文及正则错误。"""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--lepus", type=Path, default=compiler_path())
    parser.add_argument("--default", action="store_true", help="使用含标准库的默认编译路径")
    options = parser.parse_args()
    compiler = options.lepus.resolve()
    choose = '$match (A) {\nident => {\n11\n}\nexpr => {\n22\n}\ntype => {\n33\n}\nblock => {\n44\n}\nstmt => {\n55\n}\n_ => {\n66\n}\n}'
    cases = [
        ("ident", choose, "notDeclared", 11),
        ("expr", choose, "1 + 2", 22),
        ("type", choose, "int64", 33),
        ("unknown_generic_type", choose, "Unknown<int64, int32, int8>", 33),
        ("lowercase_generic_type", choose, "unknown<int64>", 33),
        ('block', '$match (A) {\nblock => {\n44\n}\n_ => {\n0\n}\n}', '{ int64 value = 1; }', 44),
        ("stmt", choose, "return 0;", 55),
        ("partial_expr", choose, "1 +", 66),
        ('full_ident', '$match (A) {\nident => {\n1\n}\n_ => {\n0\n}\n}', 'a + b', 0),
        ('branch_order', '$match (A) {\nexpr => {\n1\n}\nident => {\n2\n}\n}', 'missing', 1),
        ('ident_category', '$match (A) {\nident => {\n1\n}\nexpr => {\n2\n}\n}', 'a + b', 2),
        ('inactive_semantics', '$match (A) {\nident => {\n1\n}\n_ => {\nmissingFunction();\nint64 invalid = unknown;\n0\n}\n}', 'anything', 1),
        ('inactive_closure', '$match (A) {\nident => {\n1\n}\n_ => {\nlet unused = fn(int64 x) { missingFunction(); return x; };\n0\n}\n}', 'anything', 1),
        ('if_category', '$if (A is ident) {\n1\n} $elif (A is expr) {\n2\n} $else {\n0\n}', '1', 2),
        ('if_category_and_flag', '$if (A is ident && false) {\n1\n} $else {\n0\n}', '1', 0),
        ("regex_digits", '$regex (A is "[0-9]+") {\n1\n} $else {\n0\n}', "  123  ", 1),
        ("regex_full", '$regex (A is "[0-9]+") {\n1\n} $else {\n0\n}', "abc123", 0),
        ("regex_quotes", '$regex (A is "[0-9]+") {\n1\n} $else {\n0\n}', '"123"', 0),
        ("regex_backref", '$regex (A is "([a-z])\\\\1") {\n1\n} $else {\n0\n}', "aa", 1),
        ("regex_lookahead", '$regex (A is "(?=a)a") {\n1\n} $else {\n0\n}', "a", 1),
        ("regex_decode_once", '$regex (A is "\\\\d+") {\n1\n} $else {\n0\n}', "42", 1),
        ("regex_comment", '$regex (A is "a /\\\\*keep\\\\*/ b") {\n1\n} $else {\n0\n}', "a /*keep*/ b", 1),
        ("regex_newline", '$regex (A is "[\\\\s\\\\S]+") {\n1\n} $else {\n0\n}', "a\nb", 1),
    ]
    work = ROOT / "tmp/macro-hygiene"
    work.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="branches-", dir=work) as folder:
        directory = Path(folder)

        def execute(name, source):
            """独立编译一个分支用例，隔离所有中间产物。"""
            path = directory / (name + ".lep")
            path.write_text(source + "\n", encoding="utf-8")
            return subprocess.run([str(compiler), "run", str(path), *([] if options.default else ["--raw"]), "--out", str(directory / name)],
                                  cwd=ROOT, env=dict(os.environ, LEPUS_LANG="zh"), capture_output=True,
                                  text=True, encoding="utf-8", errors="replace", timeout=90)

        for name, body, argument, expected in cases:
            source = "$macro CHOOSE(A) -> {\n" + body + "\n}\nvoid main() { int64 result = $CHOOSE(" + argument + "); if (result == " + str(expected) + ") { return; } let __boom = 1 / (result - result); }"
            result = execute(name, source)
            assert result.returncode == 0 and not result.stderr, (name, result.returncode, result.stdout, result.stderr)
        source = '$macro EMIT(A) -> {\n$match (A) {\nstmt => {\nA;\n}\n}\n}\nvoid main() { $EMIT(return;); }'
        result = execute("emit_statement", source)
        assert result.returncode == 0 and not result.stderr, result.stderr
        source = '$macro EMPTY(A) -> {\n$match (A) {\nstmt => {\nA;\n}\n}\n}\nvoid main() { $EMPTY(1 +); return; }'
        result = execute("no_match", source)
        assert result.returncode == 0 and not result.stderr, result.stderr
        result = execute("invalid_regex", '$macro BAD(A) -> {\n$regex (A is "[") {\n1\n} $else {\n0\n}\n}\nvoid main() { let __boom = $BAD(123); }')
        assert result.returncode != 0 and "宏正则模式非法" in result.stderr, result.stderr
    print(f"宏语法分类与正则分支：{len(cases) + 3} 项 原生回归通过", flush=True)


if __name__ == "__main__":
    main()
