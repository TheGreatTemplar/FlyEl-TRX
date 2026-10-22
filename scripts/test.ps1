param([string]$ToolchainBin, [string]$PacksRoot)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'toolchain.ps1')
$taskRoot = Split-Path -Parent $PSScriptRoot
$taskTools = Resolve-FlyElToolchain -ToolchainBin $ToolchainBin -PacksRoot $PacksRoot
$taskOutput = Join-Path $taskRoot 'build/tests'
New-Item -ItemType Directory -Force -Path $taskOutput | Out-Null
foreach ($taskSource in Get-ChildItem -LiteralPath (Join-Path $taskRoot 'tests') -Filter '*_checks.c') {
    $taskIR = Join-Path $taskOutput ($taskSource.BaseName + '.ll')
    Invoke-FlyElNative $taskTools.Compiler @('--target=arm-arm-none-eabi', '-mcpu=cortex-m3', '-mthumb', '-std=c99',
        '-O3', ('-I' + (Join-Path $taskRoot 'src')), '-Wall', '-Wextra', '-Wsign-conversion', '-Werror',
        '-S', '-emit-llvm', $taskSource.FullName, '-o', $taskIR)
    # Tests include the actual production C functions. Every assertion must
    # reduce to a constant successful return, not a remaining function call.
    $taskText = Get-Content -Raw -LiteralPath $taskIR
    $taskCases = [regex]::Matches($taskText, '(?s)define[^\r\n]*@(test_\w+)\([^\r\n]*\)[^{]*\{(.*?)\r?\n\}')
    $taskExpected = [regex]::Matches((Get-Content -Raw -LiteralPath $taskSource.FullName), '(?m)^int test_\w+\(').Count
    if ($taskCases.Count -eq 0 -or $taskCases.Count -ne $taskExpected) { throw "Missing test functions: $($taskSource.Name)" }
    foreach ($taskCase in $taskCases) {
        # Constant stores retain the final state of the fake I2C bus between
        # exported test helpers; they do not represent unresolved assertions.
        $taskBody = [regex]::Replace($taskCase.Groups[2].Value,
            '(?m)^\s*store i\d+ (?:true|false|-?\d+), i\d+\* @\w+, align \d+(?:, ![^\r\n]*)?\r?\n', '')
        if ($taskBody.Trim() -notmatch '^ret i32 0$') {
            throw "Assertion not proven: $($taskCase.Groups[1].Value). Inspect $taskIR"
        }
        Write-Output "PASS $($taskCase.Groups[1].Value)"
    }
}
