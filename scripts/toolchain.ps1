# Resolve per-machine SDK locations without storing them in the repository.
function Resolve-FlyElToolchain {
    param([string]$ToolchainBin, [string]$PacksRoot)
    if (-not $ToolchainBin) {
        if ($env:FLYEL_TOOLCHAIN_BIN) { $ToolchainBin = $env:FLYEL_TOOLCHAIN_BIN }
        else {
            $taskCandidates = @('C:/Keil_v536_ARM/ARM/ARMCLANG/Bin', 'C:/Keil_v5/ARM/ARMCLANG/Bin')
            $taskCompiler = Get-Command armclang.exe -ErrorAction SilentlyContinue
            if ($taskCompiler) { $taskCandidates = @((Split-Path $taskCompiler.Source)) + $taskCandidates }
            foreach ($taskCandidate in $taskCandidates) {
                if (Test-Path -LiteralPath (Join-Path $taskCandidate 'armclang.exe')) {
                    $ToolchainBin = $taskCandidate
                    break
                }
            }
        }
    }
    if (-not $PacksRoot) {
        if ($env:CMSIS_PACK_ROOT) { $PacksRoot = $env:CMSIS_PACK_ROOT }
        else { $PacksRoot = Join-Path $env:LOCALAPPDATA 'Arm/Packs' }
    }
    if (-not $ToolchainBin) { throw 'ArmClang not found. Pass -ToolchainBin or set FLYEL_TOOLCHAIN_BIN.' }
    $taskDevice = Join-Path $PacksRoot 'Keil/STM32F1xx_DFP/2.4.1'
    $taskCore = Join-Path $PacksRoot 'ARM/CMSIS/5.8.0/CMSIS/Core/Include'
    $taskResult = [PSCustomObject]@{
        Compiler = Join-Path $ToolchainBin 'armclang.exe'
        Assembler = Join-Path $ToolchainBin 'armasm.exe'
        Linker = Join-Path $ToolchainBin 'armlink.exe'
        FromElf = Join-Path $ToolchainBin 'fromelf.exe'
        DeviceInclude = Join-Path $taskDevice 'Device/Include'
        CoreInclude = $taskCore
        Startup = Join-Path $taskDevice 'Device/Source/ARM/startup_stm32f10x_md.s'
    }
    foreach ($taskItem in $taskResult.PSObject.Properties) {
        if (-not (Test-Path -LiteralPath $taskItem.Value)) { throw "Missing SDK path: $($taskItem.Value)" }
    }
    return $taskResult
}

function Invoke-FlyElNative {
    param([string]$Executable, [string[]]$NativeArgs)
    & $Executable @NativeArgs
    if ($LASTEXITCODE -ne 0) { throw "Tool failed ($LASTEXITCODE): $Executable" }
}
