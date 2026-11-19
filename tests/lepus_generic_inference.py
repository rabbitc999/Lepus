"""验证后置泛型、枚举上下文与泛型回调在VM 中的类型推导与回调行为。"""
import os
from lepus_glue_edges import ROOT, LEPUS, EXE, run

def main():
    work = ROOT / 'tmp/lepus-generic-inference'
    work.mkdir(parents=True, exist_ok=True)
    for name in ('generic_inference', 'function_pointer_glue', 'cpp_glue'):
        source = ROOT / f'example/samples/{name}.lep'
        expected = (ROOT / f'example/expected/{name}.txt').read_text(encoding='utf-8')
        prefix = work / name
        result = run(LEPUS, 'run', source, '--out', prefix)
        assert result.stdout == expected and (not result.stderr), (name, result.stdout, result.stderr)
        run(LEPUS, 'build', source, '--emit-llvm', '--out', prefix)
if __name__ == '__main__':
    main()
