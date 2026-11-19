'use strict';

/** Windows 原生 GDB 的同步等待不处理 MI 中断时，通过已核对的目标句柄请求系统调试中断。 */
const path = require('node:path');
const { spawn } = require('node:child_process');
const { stopProcessTree } = require('./debugProtocol');

const SCRIPT = String.raw`
$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class LepusDebugInterrupt {
  [DllImport("kernel32.dll", SetLastError=true)]
  public static extern bool DebugBreakProcess(IntPtr process);
}
'@
$lepusTargetId = [int]$env:LEPUS_INTERRUPT_TARGET
$lepusDebuggerId = [int]$env:LEPUS_INTERRUPT_DEBUGGER
$lepusProcess = [System.Diagnostics.Process]::GetProcessById($lepusTargetId)
$lepusAncestors = [System.Collections.Generic.List[System.Diagnostics.Process]]::new()
try {
  $lepusHandle = $lepusProcess.Handle
  $lepusRecord = Get-CimInstance Win32_Process -Filter ('ProcessId=' + $lepusTargetId)
  if (!$lepusRecord) { throw '目标已经退出' }
  if ($env:LEPUS_INTERRUPT_START) {
    if ($lepusProcess.StartTime.ToFileTimeUtc().ToString([System.Globalization.CultureInfo]::InvariantCulture) -ne $env:LEPUS_INTERRUPT_START) { throw '附加目标创建身份已改变' }
  } else {
    $lepusAncestorId = $lepusRecord.ParentProcessId
    $lepusChild = $lepusProcess
    $lepusMatched = $false
    for ($lepusDepth = 0; $lepusDepth -lt 8 -and $lepusAncestorId -gt 0; ++$lepusDepth) {
      $lepusAncestor = [System.Diagnostics.Process]::GetProcessById($lepusAncestorId)
      $lepusAncestors.Add($lepusAncestor)
      $null = $lepusAncestor.Handle
      if ($lepusAncestor.StartTime.ToFileTimeUtc() -gt $lepusChild.StartTime.ToFileTimeUtc()) { throw '目标祖先进程编号已重用' }
      if ($lepusAncestorId -eq $lepusDebuggerId) { $lepusMatched = $true; break }
      $lepusParent = Get-CimInstance Win32_Process -Filter ('ProcessId=' + $lepusAncestorId)
      if (!$lepusParent) { throw '目标祖先进程已经退出' }
      $lepusChild = $lepusAncestor
      $lepusAncestorId = $lepusParent.ParentProcessId
    }
    if (!$lepusMatched) { throw '目标不属于当前调试器进程链' }
  }
  $lepusActual = [System.IO.Path]::GetFullPath($lepusProcess.MainModule.FileName)
  $lepusExpected = [System.IO.Path]::GetFullPath($env:LEPUS_INTERRUPT_IMAGE)
  if (![System.StringComparer]::OrdinalIgnoreCase.Equals($lepusActual, $lepusExpected)) { throw '目标映像身份已改变' }
  if ($lepusProcess.HasExited) { throw '目标已经退出' }
  if (![LepusDebugInterrupt]::DebugBreakProcess($lepusHandle)) {
    throw [System.ComponentModel.Win32Exception]::new([System.Runtime.InteropServices.Marshal]::GetLastWin32Error())
  }
} finally { foreach ($lepusAncestor in $lepusAncestors) { $lepusAncestor.Dispose() }; $lepusProcess.Dispose() }
`;

/** 对当前调试器进程链启动且映像相同的目标暂停；保留句柄并核对创建时间，防止 PID 重用。 */
async function interruptWindows(target, debuggerId, executable, environment, signal, started = () => {}, creation = null) {
  if (![target, debuggerId].every(value => Number.isSafeInteger(value) && value > 0) || typeof executable !== 'string' || !executable) throw new Error('缺少可核对的原生调试进程身份');
  if (signal?.aborted) throw new Error('暂停请求已取消');
  if (creation !== null && (typeof creation !== 'string' || !/^[0-9]+$/.test(creation))) throw new Error('附加目标创建身份无效');
  const temporary = environment?.TMPDIR;
  if (typeof temporary !== 'string' || !temporary || environment.TMP !== temporary || environment.TEMP !== temporary) throw new Error('暂停辅助进程缺少统一临时目录');
  const powershell = path.join(process.env.SystemRoot || 'C:\\Windows', 'System32/WindowsPowerShell/v1.0/powershell.exe');
  const child = spawn(powershell, ['-NoProfile', '-NonInteractive', '-Command', SCRIPT], { windowsHide: true,
    env: { ...process.env, TMPDIR: temporary, TMP: temporary, TEMP: temporary,
      LEPUS_INTERRUPT_TARGET: String(target), LEPUS_INTERRUPT_DEBUGGER: String(debuggerId), LEPUS_INTERRUPT_IMAGE: executable,
      LEPUS_INTERRUPT_START: creation || '' } });
  started(child);
  let failure, output = '';
  const stop = error => { failure ||= error; void stopProcessTree(child).catch(error => { failure ||= error; }); };
  const abort = () => stop(new Error('暂停请求已取消'));
  const collect = bytes => { if (output.length + bytes.length > 16384) stop(new Error('暂停辅助进程输出超过预算')); else output += bytes.toString('utf8'); };
  child.stdout.on('data', collect); child.stderr.on('data', collect);
  signal?.addEventListener('abort', abort, { once: true });
  const timer = setTimeout(() => stop(new Error('暂停辅助进程超时')), 10000);
  try {
    const status = await new Promise((resolve, reject) => { child.once('error', reject); child.once('close', resolve); });
    if (failure) throw failure;
    if (status !== 0) throw new Error('无法暂停原生目标：' + output.trim().slice(0, 4096));
  } finally {
    clearTimeout(timer); signal?.removeEventListener('abort', abort);
    await stopProcessTree(child);
  }
}

module.exports = { interruptWindows };
