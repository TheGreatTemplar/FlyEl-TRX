param([string]$Python = 'python', [string]$ToolchainBin, [string]$PacksRoot)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'toolchain.ps1')
$taskTools = Resolve-FlyElToolchain -ToolchainBin $ToolchainBin -PacksRoot $PacksRoot
& $Python (Join-Path $PSScriptRoot 'run_arm_tests.py') --compiler $taskTools.Compiler --linker $taskTools.Linker --include $taskTools.CoreInclude --include $taskTools.DeviceInclude
if ($LASTEXITCODE -ne 0) { throw 'ARM runtime checks failed.' }
