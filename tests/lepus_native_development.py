"""无 Python 运行环境下的原生开发命令、普通 LSP 宏提示和真实展开预览回归。"""
from compiler_path import compiler_path
from pathlib import Path
import json
import os
import subprocess
from project_temp import tempfile
import shutil
import sys
ROOT = Path(__file__).resolve().parents[1]
COMPILER = Path(os.environ.get('LEPUS_TEST_COMPILER', compiler_path())).resolve()


def native_runtime_available(compiler):
    """判断宿主是否有 clang，决定 lepus test 能真正运行程序还是只做语义检查。"""
    clang = os.environ.get('LEPUS_LLVM_CLANG') or shutil.which('clang')
    if clang:
        return Path(clang).is_file()
    return any((compiler.parent / relative / 'clang.exe').is_file() for relative in
               ('build/dependencies/llvm/toolchain/bin', 'llvm/bin', 'build/dependencies/llvm/static-sdk/bin'))


def toolchain_directories(compiler):
    """返回编译器所在目录下可用的 LLVM 工具链目录，供受限 PATH 保留原生构建能力。"""
    return [str(compiler.parent / relative) for relative in
            ('build/dependencies/llvm/toolchain/bin', 'llvm/bin', 'build/dependencies/llvm/static-sdk/bin')
            if (compiler.parent / relative / 'clang.exe').is_file()]


def main():
    """禁用解释器并用真实 JSON-RPC 校验全部迁移入口。"""
    with tempfile.TemporaryDirectory(prefix='原生开发工具 ') as directory:
        root = Path(directory)
        env = dict(os.environ, LEPUS_PYTHON=str(root / 'missing-python.exe'))
        if os.name == 'nt':
            runtime = Path(shutil.which('g++')).parent
            assert not (runtime / 'python.exe').exists()
            # 受限 PATH 只保留系统目录、C++ 运行时和已安装的 LLVM 工具链，仍不含 Python。
            env['PATH'] = os.pathsep.join([str(Path(os.environ['SystemRoot']) / 'System32'), str(runtime),
                                           *toolchain_directories(COMPILER)])
        else:
            env['PATH'] = os.pathsep.join(['/usr/bin', '/bin', *toolchain_directories(COMPILER)])
        assert shutil.which('python', path=env['PATH']) is None
        assert shutil.which('python3', path=env['PATH']) is None

        def run(*args, code=0, **kwargs):
            result = subprocess.run([str(COMPILER), *map(str, args)], cwd=root, env=env, capture_output=True, timeout=90, **kwargs)
            assert result.returncode == code, (args, result.stdout, result.stderr)
            return result
        (root / 'lepus.toml').write_text('[package]\nname="native"\ncompiler=">=0.9.1"\n', encoding='utf-8')
        run('deps', 'lock', '--offline')
        run('deps', 'check', '--offline')
        source = root / 'main.lep'
        source.write_text('void main(){\nreturn;\n}\n', encoding='utf-8')
        run('fmt', source, '--check', code=1)
        run('fmt', source, '--write')
        run('fmt', source, '--check')
        test = root / 'test_pass.lep'
        test.write_text('void main(){std::core::println("通过");}', encoding='utf-8')
        test.with_suffix('.expected').write_text('通过\n', encoding='utf-8')
        skipped = root / 'test_skip.lep'
        skipped.write_text('invalid', encoding='utf-8')
        skipped.with_suffix('.test.toml').write_text('skip="测试跳过"\n', encoding='utf-8')
        report = json.loads(run('test', root, '--json').stdout)
        # 没有 clang 时无法构建原生程序，lepus test 只做语义检查并标记跳过。
        runnable = native_runtime_available(COMPILER)
        expected_pass = 'passed' if runnable else 'skipped'
        assert report['summary'] == dict(passed=int(runnable), failed=0, skipped=1 + int(not runnable)), report
        assert report['tests'][0]['status'] == expected_pass, report
        timed = root / 'test_timeout.lep'
        timed.write_text('void main(){while(true){}}', encoding='utf-8')
        report = json.loads(run('test', timed, '--timeout', '0.1', '--json', code=0 if not runnable else 1).stdout)
        if runnable:
            assert 'timed out' in report['tests'][0]['message'], report
        else:
            assert report['tests'][0]['status'] == 'skipped', report
        module = root / 'calc.lep'
        module.write_text('module calc;\n/// 整数文档\nint64 add(int64 a,int64 b=1){return a+b;}\n/// 浮点文档\nfloat64 add(float64 a,float64 b){return a+b;}\nstruct Box { /// 字段文档\nint64 value; };\n', encoding='utf-8')
        included = root / 'macros.lep'
        included.write_text('module macros;\n/// 包含宏文档\n$macro TWICE(x) -> {\n((x)+(x))\n}\n', encoding='utf-8')
        text = 'import calc;\nimport std::math;\nimport macros;\n/// 平方宏文档\n$macro SQUARE(x) -> {\nx*x\n}\n/// 单行宏文档\n$replace PLUS(x,y) x+y\n$macro ANSWER -> {\n3\n}\nvoid main(){\n    String emoji="😀";\n    calc::Box box;\n    box.value;\n    calc::add(1,2);\n    std::math::sqrt(4.0);\n    int64 answer=$SQUARE($TWICE(2))+$PLUS(1,2)+$ANSWER;\n    let enabled = $!FEATURE_A && !$!FEATURE_A;\n    let __boom = answer;\n}\n'
        source.write_text(text, encoding='utf-8')

        def position(at):
            return dict(line=text.count('\n', 0, at), character=len(text[text.rfind('\n', 0, at) + 1:at].encode('utf-16-le')) // 2)
        requests = [dict(id=1, method='initialize', params={'rootUri': root.as_uri()}), dict(method='textDocument/didOpen', params={'textDocument': dict(uri=source.as_uri(), text=text, version=1)})]

        def query(id, method, needle='', delta=0):
            params = {'textDocument': {'uri': source.as_uri()}}
            if needle:
                params['position'] = position(text.index(needle) + delta)
            requests.append(dict(id=id, method=method, params=params))
        query(2, 'textDocument/hover', 'calc::add', 8)
        query(3, 'textDocument/signatureHelp', 'calc::add(1,', 12)
        query(4, 'textDocument/completion', 'box.', 4)
        query(5, 'textDocument/definition', 'box.value', 7)
        query(6, 'textDocument/hover', '$SQUARE($TWICE', 2)
        query(7, 'textDocument/signatureHelp', '$SQUARE($TWICE', len('$SQUARE('))
        query(8, 'textDocument/definition', '$TWICE(2)', 2)
        query(9, 'lepus/expandMacro')
        query(10, 'textDocument/semanticTokens/full')
        query(11, 'textDocument/documentSymbol')
        query(12, 'textDocument/hover', 'sqrt', 2)
        query(14, 'textDocument/hover', '$ANSWER;', 3)
        query(15, 'textDocument/signatureHelp', '$PLUS(1,', len('$PLUS(1,'))
        query(16, 'textDocument/hover', '$!FEATURE_A &&', 1)
        requests.extend([dict(id=13, method='shutdown'), dict(method='exit')])
        payload = b''
        for request in requests:
            raw = json.dumps(dict(jsonrpc='2.0', **request), ensure_ascii=False).encode()
            payload += f'Content-Length: {len(raw)}\r\n\r\n'.encode() + raw
        result = run('lsp', input=payload)
        frames = result.stdout
        responses = {}
        while frames:
            header, _, rest = frames.partition(b'\r\n\r\n')
            assert _, frames[:300]
            size = int(header.split(b':', 1)[1])
            message = json.loads(rest[:size])
            frames = rest[size:]
            if 'id' in message:
                responses[message['id']] = message

        def value(id):
            assert 'error' not in responses[id], responses[id]
            return responses[id]['result']
        assert value(1)['serverInfo']['version'] == (ROOT / 'VERSION').read_text().strip(), value(1)
        assert '整数文档' in value(2)['contents']['value'] and '浮点文档' not in value(2)['contents']['value']
        assert value(3)['activeParameter'] == 1 and len(value(3)['signatures']) == 2, value(3)
        assert any((item['label'] == 'value' for item in value(4))), value(4)
        assert value(5)[0]['uri'] == module.as_uri(), value(5)
        assert '平方宏文档' in value(6)['contents']['value'], value(6)
        signature = value(7)['signatures'][0]
        label = signature['parameters'][0]['label']
        parameter = signature['label'][label[0]:label[1]] if isinstance(label, list) else label
        assert parameter.strip('()') == 'x', value(7)
        assert value(8)[0]['uri'] == included.as_uri(), value(8)
        assert value(9)['diagnostics'] == [], value(9)
        expanded = value(9)['text']
        assert json.loads(expanded)['format'] == 'hygienic-ast', expanded
        assert any(item['name'] == 'SQUARE' for item in json.loads(expanded)['expansions']), expanded
        assert '$macro ANSWER' in value(14)['contents']['value']
        assert value(15)['activeParameter'] == 1 and 'PLUS' in value(15)['signatures'][0]['label']
        assert value(10)['data'] and any((item['name'] == 'SQUARE' for item in value(11)))
        flag = '$!FEATURE_A &&'
        flag_at = text.index(flag)
        legend = value(1)['capabilities']['semanticTokensProvider']['legend']
        tokens = {}
        line = column = 0
        for offset in range(0, len(value(10)['data']), 5):
            delta_line, delta_column, length, kind, modifiers = value(10)['data'][offset:offset + 5]
            line += delta_line
            column = delta_column if delta_line else column + delta_column
            tokens[(line, column)] = (length, legend['tokenTypes'][kind], modifiers)
        point = position(flag_at)
        assert tokens[(point['line'], point['character'])] == (len('$!FEATURE_A'), 'variable', 2)
        assert 'build' in value(16)['contents']['value'], value(16)
        assert 'sqrt' in value(12)['contents']['value']
        assert value(9)['format'] == 'json' and value(9)['mappings'], value(9)
    print('原生 LSP、宏提示/展开、格式化、依赖锁定及测试/超时全部通过，无外部 Python。')
if __name__ == '__main__':
    main()
