"""Verify Windows PE application subsystems and project option precedence."""

import argparse
import os
from pathlib import Path
import struct
import subprocess

from compiler_path import compiler_path
from project_temp import tempfile


def subsystem(path):
    """Read the subsystem field of the generated x64 PE executable."""
    data = path.read_bytes()
    assert data[:2] == b'MZ', path
    pe, = struct.unpack_from('<I', data, 0x3c)
    assert data[pe:pe + 6] == b'PE\0\0\x64\x86', path
    assert struct.unpack_from('<H', data, pe + 24)[0] == 0x20b, path
    return struct.unpack_from('<H', data, pe + 24 + 68)[0]


def main():
    """Run Windows application exports, a separate link, and invalid options."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--lepus', type=Path, default=compiler_path())
    args = parser.parse_args()
    if os.name != 'nt':
        print('Windows PE subsystem acceptance requires Windows.')
        return
    compiler = args.lepus.resolve()
    with tempfile.TemporaryDirectory(prefix='windows-subsystem-') as directory:
        work = Path(directory)
        source = work / 'app.lep'
        source.write_text('void main() {}\n', encoding='utf-8')

        def lepus(*arguments, error=None):
            """Invoke the selected compiler and verify success or a diagnostic."""
            result = subprocess.run([compiler, *map(str, arguments)], cwd=work, capture_output=True,
                                    encoding='utf-8', errors='replace', timeout=180)
            if error is None:
                assert result.returncode == 0, (arguments, result)
            else:
                assert result.returncode != 0 and error in result.stderr, (arguments, result)

        lepus('build', source, '--out', work / 'default')
        assert subsystem(work / 'default.exe') == 3
        lepus('build', source, '--subsystem', 'windows', '--out', work / 'gui')
        assert subsystem(work / 'gui.exe') == 2
        subprocess.run([work / 'gui.exe'], cwd=work, check=True, timeout=15)
        lepus('build', source, '--emit-obj', '--out', work / 'object')
        lepus('link', work / 'object.o', '--subsystem', 'windows', '--out', work / 'linked.exe')
        assert subsystem(work / 'linked.exe') == 2
        lepus('build', source, '--subsystem', 'bad', error='--subsystem')
        lepus('build', source, '--subsystem', 'windows', '--shared', error='--subsystem')
        lepus('build', source, '--subsystem', 'windows', '--target', 'linux-x64', error='--subsystem')
        (work / 'lepus.toml').write_text('[lepus]\nmain = "app.lep"\n[build]\nsubsystem = "windows"\n', encoding='utf-8')
        lepus('build', source, '--out', work / 'project')
        assert subsystem(work / 'project.exe') == 2
        lepus('build', source, '--subsystem', 'console', '--out', work / 'override')
        assert subsystem(work / 'override.exe') == 3
        lepus('run', source)
    print('Windows PE default, GUI, standalone link, project configuration and console override passed.')


if __name__ == '__main__':
    main()
