"""验证宏条件链的两个拼写、惰性选择、成员模板和用户 JSON_FIELDS 示例。"""
from pathlib import Path
import argparse
import json
import os
import subprocess
from compiler_path import compiler_path
from project_temp import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    """真实展开和执行分支链，负例分别核对条件、顺序及结构错误。"""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--lepus', type=Path, default=compiler_path())
    compiler = parser.parse_args().lepus.resolve()
    fixture = (ROOT / 'tests/fixtures/macro_else_if.lep').read_text(encoding='utf-8')
    definitions = fixture[:fixture.index('\nstruct Data')]
    lazy = '''$macro PICK() -> expr {
$if (true) {
7
} $elif (missingCondition > 0) {
$compile_error("not selected");
missingValue
} $else {
missingFunction()
}
}
void main(){assert($PICK()==7);}
'''
    member = '''$macro METHODS(gname:ident,n:expr) -> member {
$if (n==1) {
int64 gname(){return 11;}
} $else_if (n==2) {
int64 gname(){return 22;}
} $elif (n==3) {
int64 gname(){return 33;}
} $else {
int64 gname(){return 44;}
}
}
struct First{$METHODS(read,2);}
struct Second{$METHODS(read,3);}
struct Last{$METHODS(read,4);}
void main(){First a;Second b;Last c;assert(a.read()==22&&b.read()==33&&c.read()==44);}
'''
    with tempfile.TemporaryDirectory(prefix='macro-else-if-') as directory:
        work = Path(directory)

        def invoke(name, text, success=True, diagnostic=''):
            """隔离源码和产物，检查完整诊断，成功场景实际运行。"""
            source = work / (name + '.lep')
            source.write_text(text, encoding='utf-8')
            result = subprocess.run([str(compiler), 'run' if success else 'check', str(source), '--out', str(work / name)],
                                    cwd=ROOT, env=dict(os.environ, LEPUS_LANG='zh'), capture_output=True,
                                    text=True, encoding='utf-8', errors='replace', timeout=180)
            assert (result.returncode == 0) == success, (name, result)
            if success:
                assert not result.stderr, (name, result)
            elif diagnostic:
                assert diagnostic in result.stderr, (name, result)
            return result

        for alias, text in [('else_if', fixture), ('elif', fixture.replace('$else_if', '$elif')),
                            ('mixed', fixture.replace('$else_if', '$elif', 1))]:
            result = invoke(alias, text)
            assert json.loads(result.stdout) == dict(title='Lepus', count=42, enabled=True), result
        invoke('lazy', lazy)
        invoke('members', member)
        invoke('nested', '''$macro SELECT(n:expr) -> expr {
$if (n==1) {
11
} $elif (n==2) {
$if (false) {
missingValue
} $elif (true) {
22
} $else {
missingOther
}
} $else {
33
}
}
void main(){assert($SELECT(1)==11&&$SELECT(2)==22&&$SELECT(3)==33);}
''')
        invoke('unsupported_field', definitions + '\nstruct Data{float64 unsupported;}\nvoid main(){Data value;$JSON_FIELDS<Data>(value);}',
               False, 'JSON_FIELDS 不支持该字段类型: float64')
        invoke('empty_fields', definitions + '\nstruct Data{}\nvoid main(){Data value;$JSON_FIELDS<Data>(value);}',
               False, 'JSON 生成至少需要一个实例字段')
        invoke('invalid_selected_condition', lazy.replace('$if (true) {', '$if (false) {'), False, '常量表达式不能引用运行时变量：missingCondition')
        invoke('non_boolean_condition', lazy.replace('$if (true) {', '$if (false) {').replace('missingCondition > 0', '42'), False, '条件必须')
        for name, text in {
            'orphan_elif': '$elif true\nvoid main(){}',
            'orphan_else_if': '$else_if true\nvoid main(){}',
            'after_else': lazy.replace('} $elif (missingCondition > 0) {', '} $else {\n1\n} $elif (true) {'),
            'missing_condition': lazy.replace('} $elif (missingCondition > 0) {', '} $elif {'),
            'missing_end': lazy.replace('} $elif', '$elif', 1),
        }.items():
            result = invoke(name, text, False)
            expected = '花括号宏定义缺少右花括号' if name == 'missing_end' else name + '.lep'
            assert expected in result.stderr, (name, result)
    print('macro conditions: elif/else_if JSON_FIELDS, lazy selection, nested and member chains, diagnostics passed')


if __name__ == '__main__':
    main()
