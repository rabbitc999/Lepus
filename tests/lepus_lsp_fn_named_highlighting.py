"""验证 fn 前缀（返回类型后置）具名函数的原生语义着色。"""
from pathlib import Path
import os

from compiler_path import compiler_path
from native_lsp_transport import Transport, position
from project_temp import tempfile

ROOT = Path(__file__).resolve().parents[1]
COMPILER = Path(os.environ.get('LEPUS_TEST_COMPILER', compiler_path())).resolve()


def main():
    """声明名、类型参数与形参按声明着色，lambda 和函数类型不被误认作声明。"""
    text = '''type alias fx<T> = T fn(T);

fn compose<T>(fx<T> a, fx<T> b) -> fx<T> {
    return fn move(T x) { a(b(x)) };
}

fx<T> add<T>(fx<T> a, fx<T> b) {
    return fn move(T x) { a(x) + b(x) };
}

fn add2(int64 left, int64 right) -> int64 { return left + right; }
fn show() { println("void"); }
fn makeAdder<T>(T base) -> (T fn(T)) {
    return T fn copy (T value) { return base + value; };
}

void main() {
    int64 fn(int64) twice = fn(int64 x) { x * 2 };
    println("{}", compose(add, add)(3));
}
'''
    with tempfile.TemporaryDirectory(prefix='lepus-lsp-fn-named-') as folder:
        root = Path(folder)
        uri = (root / 'main.lep').as_uri()
        server = Transport(COMPILER, root)
        try:
            init = server.query('initialize', dict(rootUri=root.as_uri()))
            legend = init['capabilities']['semanticTokensProvider']['legend']['tokenTypes']
            server.send(dict(method='textDocument/didOpen', params=dict(
                textDocument=dict(uri=uri, version=1, text=text))))
            data = server.query('textDocument/semanticTokens/full',
                                dict(textDocument=dict(uri=uri)))['data']
            tokens = {}
            line = column = 0
            for offset in range(0, len(data), 5):
                delta_line, delta_column, length, kind, modifiers = data[offset:offset + 5]
                line += delta_line
                column = delta_column if delta_line else column + delta_column
                tokens[(line, column)] = (length, legend[kind], modifiers)

            def role(needle, start=0):
                """取得指定源码片段起点的语义着色类别与修饰符。"""
                point = position(text, text.index(needle, start))
                return tokens[(point['line'], point['character'])][1:]

            # fn 前缀具名函数的声明名、类型参数与形参。
            assert role('compose<T>') == ('function', 1)
            assert role('T>(fx<T> a') == ('typeParameter', 1)
            assert role('a, fx<T> b') == ('parameter', 1)
            assert role('b) -> fx<T>') == ('parameter', 1)
            # 同样的声明改成前置返回类型，逐项类别必须一致。
            add = text.index('fx<T> add<T>')
            assert role('add<T>', add) == ('function', 1)
            assert role('T>(fx<T> a', add) == ('typeParameter', 1)
            assert role('a, fx<T> b', add) == ('parameter', 1)
            assert role('b) {', add) == ('parameter', 1)
            # 无泛型、省略箭头、返回闭包类型的 fn 前缀函数同样是函数声明。
            assert role('add2(int64') == ('function', 1)
            assert role('show()') == ('function', 1)
            assert role('makeAdder<T>') == ('function', 1)
            assert role('T>(T base) -> (T fn(T))') == ('typeParameter', 1)
            assert role('left, int64 right') == ('parameter', 1)
            assert role('base) -> (T fn(T))') == ('parameter', 1)
            # lambda 捕获、匿名 lambda、函数类型里的 fn 仍是关键字，不产生函数名。
            assert role('fn move') == ('keyword', 0)
            assert role('fn copy') == ('keyword', 0)
            assert role('fn(int64 x)') == ('keyword', 0)
            assert role('fn(T);') == ('keyword', 0)
            assert role('x) { a(b(x)) }') == ('parameter', 1)
            # 调用处仍是函数，但不带 declaration 修饰。
            assert role('compose(add') == ('function', 0)
        finally:
            server.close()
    print('fn 前缀具名函数的声明名、类型参数、形参与 lambda 边界语义着色通过。')


if __name__ == '__main__':
    main()
