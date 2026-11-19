#!/usr/bin/env node
import { copyFile, mkdir, readFile, stat, writeFile } from 'node:fs/promises'
import { createHash } from 'node:crypto'
import { dirname, join, resolve, isAbsolute, relative } from 'node:path'
import { spawn } from 'node:child_process'
import { fileURLToPath } from 'node:url'

const here = dirname(fileURLToPath(import.meta.url)); const root = resolve(here, '..'); const repo = resolve(root, '..', '..')
const b = spawn(process.execPath, [join(root, 'scripts', 'build.mjs')], { stdio: 'inherit' }); await new Promise((r) => b.on('exit', r)); if (b.exitCode) process.exit(b.exitCode)
const candidates = [process.env.LEPUS_COMPILER, process.env.LEPUS_BIN, join(repo, 'vscode', 'bin', 'lepus.exe'), join(repo, 'vscode', 'bin', 'lepus'), 'lepus.exe', 'lepus']
let source
for (const c of candidates.filter(Boolean)) { const p = isAbsolute(c) ? c : resolve(process.cwd(), c); try { if ((await stat(p)).isFile()) { source = p; break } } catch {} }
if (!source) { console.error('No Lepus compiler found; set LEPUS_COMPILER or build vscode/bin/lepus.exe first.'); process.exit(1) }
const bin = join(root, 'bin'); await mkdir(bin, { recursive: true }); const dest = join(bin, process.platform === 'win32' ? 'lepus.exe' : 'lepus'); await copyFile(source, dest)
const hash = createHash('sha256'); hash.update(await readFile(dest)); const version = (await readFile(join(repo, 'VERSION'), 'utf8').catch(() => 'unknown')).trim(); await writeFile(join(bin, 'compiler.json'), JSON.stringify({ version, source, file: `bin/${dest.split(/[\\/]/).pop()}`, bytes: (await stat(dest)).size, sha256: hash.digest('hex') }, null, 2) + '\n')
console.log(`packaged ${dest}`)
