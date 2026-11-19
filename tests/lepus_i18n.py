"""验收 Lepus 命名、双语工具提示、系统区域选择及程序输出隔离。"""
from compiler_path import compiler_path
from pathlib import Path
import argparse
import json
import os
import subprocess
from project_temp import tempfile
ROOT = Path(__file__).resolve().parents[1]
WORK = ROOT / 'build/lepus-i18n'

def execute(*args, language=None, extra_env=None):
    """隔离区域环境，按 UTF-8 读取本地工具输出。"""
    env = os.environ.copy()
    for key in ('LEPUS_LANG', 'LC_ALL', 'LC_MESSAGES', 'LANG'):
        env.pop(key, None)
    if language is not None:
        env['LEPUS_LANG'] = language
    if extra_env:
        env.update(extra_env)
    return subprocess.run([str(x) for x in args], cwd=ROOT, env=env, capture_output=True, encoding='utf-8', errors='strict', timeout=180)

def main():
    """检查解释、VM、工程模板和两种显示语言。"""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--lepus', type=Path, default=compiler_path())
    compiler = parser.parse_args().lepus.resolve()
    WORK.mkdir(parents=True, exist_ok=True)
    unit = WORK / 'unit.exe'
    result = execute('g++', '-std=c++17', '-finput-charset=UTF-8', '-fexec-charset=UTF-8', '-I', ROOT / 'library', ROOT / 'tests/lepus_i18n_test.cpp', '-o', unit)
    assert result.returncode == 0, result
    assert execute(unit).returncode == 0
    expected = {'zh': '用法：', 'en': 'Usage:'}
    for language, token in expected.items():
        result = execute(compiler, 'help', 'lang', language)
        assert result.returncode == 0 and token in result.stdout, (language, result)
        assert 'lang zh|en|auto' in result.stdout
        assert 'Magic-C' not in result.stdout and '.mc ' not in result.stdout
        result = execute(compiler, 'lang', language, 'new', 'help')
        assert result.returncode == 0 and token in result.stdout, result
        result = execute(compiler, 'test', 'help', 'lang', language)
        assert result.returncode == 0 and '--backend' in result.stdout, result
        if language == 'en':
            assert 'Usage:' in result.stdout, result
    for environment, token in [({'LC_ALL': 'ja_JP.UTF-8'}, expected['en']), ({'LC_MESSAGES': 'zh_TW.UTF-8'}, expected['zh']), ({'LANG': 'C'}, expected['en']), ({'LANG': 'fr_FR'}, expected['en']), ({'LEPUS_LANG': 'ja', 'LC_ALL': 'zh_CN'}, expected['en'])]:
        result = execute(compiler, 'help', extra_env=environment)
        assert result.returncode == 0 and token in result.stdout, (environment, result)
    result = execute(compiler, 'help', 'lang', 'en', extra_env={'LEPUS_LANG': 'ja'})
    assert expected['en'] in result.stdout, result
    result = execute(compiler, 'help', 'lang', 'auto', extra_env={'LEPUS_LANG': 'en', 'LC_ALL': 'ja_JP'})
    assert expected['en'] in result.stdout, result
    result = execute(compiler, 'lang', 'fr', 'new', WORK / 'must-not-exist')
    assert result.returncode == 2 and (not (WORK / 'must-not-exist').exists()), result
    rejected = execute(compiler, 'help', 'lang', 'ja')
    assert rejected.returncode == 2, rejected
    result = execute(compiler, '--lang')
    assert result.returncode == 2, result
    source = WORK / '中日文路径.lep'
    source.write_text('void main(){std::core::println("中文 / 日本語 / Lepus");}\n', encoding='utf-8')
    for language in expected:
        result = execute(compiler, source, '--lang', 'ja', language=language)
        assert result.returncode == 0 and result.stdout == '中文 / 日本語 / Lepus\n' and (not result.stderr), result
    bad = WORK / 'readonly.lep'
    bad.write_text('void main(){int64 x=1;int64 & r=&x;r=2;}\n', encoding='utf-8')
    codes = []
    for language, word in {'zh': '只读', 'en': 'read-only'}.items():
        result = execute(compiler, 'check', bad, 'json', 'lang', language)
        assert result.returncode != 0, result
        report = json.loads(result.stdout)['diagnostics'][0]
        assert word in report['message'], report
        assert report['source'] == bad.read_text(encoding='utf-8').strip(), report
        codes.append(report['code'])
    assert codes == ['LEP1203'] * 2, codes
    unknown = WORK / 'unknown.lep'
    unknown.write_text('void main(){string x;}\n', encoding='utf-8')
    for language, word in {'zh': '没有声明的类型', 'en': 'Undeclared type'}.items():
        result = execute(compiler, 'check', unknown, 'json', 'lang', language)
        assert result.returncode != 0 and word in result.stdout and ('string' in result.stdout), result
    errors = ROOT / 'example/samples-error'
    for fixture, translations in [
        ('macro_body_backtick.lep', {'zh': '认不出来的字符', 'en': 'Unrecognized character'}),
        ('collection_missing_value.lep', {'zh': '期望 ', 'en': 'expected '}),
    ]:
        path = errors / fixture
        for language, word in translations.items():
            heading = '语法错误：' if language == 'zh' else 'Syntax error: '
            result = execute(compiler, 'check', path, '--lang', language,
                             language='en' if language == 'zh' else 'zh')
            assert result.returncode != 0 and result.stderr.startswith(heading) and word in result.stderr, result
            result = execute(compiler, 'check', path, '--diagnostics=json', language=language)
            assert result.returncode != 0 and not result.stderr, result
            [report] = json.loads(result.stdout)['diagnostics']
            assert report['code'] == 'LEP1001' and word in report['message'], report
            assert Path(report['span']['file']).resolve() == path, report
    unterminated = WORK / 'unterminated.lep'
    unterminated.write_text('void main() { String text = "unclosed; }\n', encoding='utf-8')
    for language, word in {'zh': '插值：字符串缺少结束引号',
                           'en': 'Interpolation: Missing closing quote in string or character literal'}.items():
        result = execute(compiler, 'check', unterminated, language=language)
        assert result.returncode != 0 and word in result.stderr, result
    with tempfile.TemporaryDirectory(dir=WORK) as directory:
        target = Path(directory) / 'hello-lepus'
        result = execute(compiler, 'new', target, 'lang', 'en')
        assert result.returncode == 0 and 'Created' in result.stdout, result
        assert (target / 'src/main.lep').exists() and (target / 'lepus.toml').exists()
        assert 'Lepus application' in (target / 'README.md').read_text(encoding='utf-8')
        result = execute(compiler, 'new', target, 'lang', 'en')
        assert result.returncode != 0 and 'already exists' in result.stderr, result
    native = WORK / 'native.exe'
    panic = WORK / 'panic.lep'
    panic.write_text('void leaf(){panic("USER 中文 / 日本語");}\nvoid main(){leaf();return;}\n', encoding='utf-8')
    for language, word in {'zh': '调用栈', 'en': 'Call stack'}.items():
        result = execute(compiler, 'run', panic, language=language)
        assert result.returncode != 0 and 'USER 中文 / 日本語' in result.stderr and (word in result.stderr), result
    checked = WORK / 'checked.exe'
if __name__ == '__main__':
    main()
