param([string]$ToolchainBin, [string]$PacksRoot)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'toolchain.ps1')
$taskRoot = Split-Path -Parent $PSScriptRoot
$taskTools = Resolve-FlyElToolchain -ToolchainBin $ToolchainBin -PacksRoot $PacksRoot
$taskOutput = Join-Path $taskRoot 'build'
New-Item -ItemType Directory -Force -Path $taskOutput | Out-Null
& (Join-Path $PSScriptRoot 'update-build-info.ps1') -Root $taskRoot
$taskSources = @(Get-ChildItem -LiteralPath (Join-Path $taskRoot 'src') -Filter '*.c' | Sort-Object Name)
$taskObjects = @()
$taskCommon = @('--target=arm-arm-none-eabi', '-mcpu=cortex-m3', '-mthumb', '-fshort-enums', '-fshort-wchar',
    '-std=c99', '-O1', '-ffunction-sections', '-DSTM32F10X_MD',
    ('-I' + $taskTools.CoreInclude), ('-I' + $taskTools.DeviceInclude), ('-I' + (Join-Path $taskRoot 'src')),
    '-Wall', '-Wextra', '-Wsign-conversion', '-Wpadded', '-Werror')
foreach ($taskSource in $taskSources) {
    $taskObject = Join-Path $taskOutput ($taskSource.BaseName + '.o')
    Invoke-FlyElNative $taskTools.Compiler ($taskCommon + @('-c', $taskSource.FullName, '-o', $taskObject))
    $taskObjects += $taskObject
}
$taskStartupObject = Join-Path $taskOutput 'startup_stm32f10x_md.o'
Invoke-FlyElNative $taskTools.Assembler @('--cpu', 'Cortex-M3', '--thumb', $taskTools.Startup, '-o', $taskStartupObject)
$taskObjects += $taskStartupObject
$taskAxf = Join-Path $taskOutput 'FlyEl.axf'
$taskHex = Join-Path $taskOutput 'FlyEl.hex'
Invoke-FlyElNative $taskTools.Linker (@('--cpu', 'Cortex-M3', '--strict', '--scatter',
    (Join-Path $taskRoot 'firmware/stm32f103c8.sct'), '--summary_stderr', '--info', 'summarysizes',
    '--map', '--list', (Join-Path $taskOutput 'FlyEl.map')) + $taskObjects + @('-o', $taskAxf))
Invoke-FlyElNative $taskTools.FromElf @('--i32combined', '--output', $taskHex, $taskAxf)
Write-Output "Built $($taskSources.Count) C modules without warnings. HEX: $taskHex"
