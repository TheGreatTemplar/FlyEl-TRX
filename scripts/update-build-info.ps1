param([string]$Root = (Split-Path -Parent $PSScriptRoot))
$ErrorActionPreference = 'Stop'

function Resolve-FlyElGit {
    if ($env:FLYEL_GIT) {
        if (-not (Test-Path -LiteralPath $env:FLYEL_GIT -PathType Leaf)) {
            throw "FLYEL_GIT points to a missing executable: $env:FLYEL_GIT"
        }
        return (Resolve-Path -LiteralPath $env:FLYEL_GIT).ProviderPath
    }
    $taskCommand = Get-Command git.exe -CommandType Application -ErrorAction SilentlyContinue
    if ($taskCommand) { return $taskCommand.Source }
    $taskCandidates = @()
    foreach ($taskBase in @($env:ProgramFiles, ${env:ProgramFiles(x86)})) {
        if ($taskBase) { $taskCandidates += Join-Path $taskBase 'Git/cmd/git.exe' }
    }
    if ($env:LOCALAPPDATA) { $taskCandidates += Join-Path $env:LOCALAPPDATA 'Programs/Git/cmd/git.exe' }
    if ($env:USERPROFILE) {
        $taskCandidates += Join-Path $env:USERPROFILE 'scoop/apps/git/current/cmd/git.exe'
        $taskCandidates += Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/native/git/cmd/git.exe'
    }
    foreach ($taskCandidate in $taskCandidates) {
        if (Test-Path -LiteralPath $taskCandidate -PathType Leaf) { return $taskCandidate }
    }
    throw 'Git not found. Install Git for Windows or set FLYEL_GIT to the full path of git.exe, then rebuild.'
}

function Get-FlyElGitValue {
    param([string]$Repository, [string[]]$Arguments, [string]$Fallback)
    $taskGit = Resolve-FlyElGit
    $taskPreference = $ErrorActionPreference
    try {
        # Windows PowerShell 5.1 wraps native stderr in ErrorRecord; check the process exit code explicitly.
        $ErrorActionPreference = 'Continue'
        $taskValue = & $taskGit -C $Repository @Arguments 2>&1
        $taskSucceeded = $?
        $taskExit = $LASTEXITCODE
    }
    finally { $ErrorActionPreference = $taskPreference }
    if (-not $taskSucceeded -or $taskExit -ne 0) {
        if ($PSBoundParameters.ContainsKey('Fallback')) { return $Fallback }
        throw ("Git metadata failed ({0}): {1}" -f ($Arguments -join ' '), (@($taskValue) -join "`n"))
    }
    return (@($taskValue) -join "`n").Trim()
}

function ConvertTo-FlyElCString {
    param([string]$Value, [int]$Limit = 200)
    $taskText = [regex]::Replace($Value, '\s+', ' ').Trim()
    $taskText = [regex]::Replace($taskText, '[^\x20-\x7e]', '?')
    if ($taskText.Length -gt $Limit) { $taskText = $taskText.Substring(0, $Limit - 1) + '+' }
    return '"' + $taskText.Replace('\', '\\').Replace('"', '\"') + '"'
}

function Get-FlyElSourceHash {
    param([string]$Repository)
    $taskPathsText = Get-FlyElGitValue $Repository @('-c','core.quotepath=false','ls-files','--cached','--others',
        '--exclude-standard','--','src','font','scripts','firmware','gmg12864_v01.uvprojx')
    [string[]]$taskPaths = @($taskPathsText -split "`n" | Where-Object { $_ -match '\.(c|h|ps1|py|sct|uvprojx)$' })
    if ($taskPaths.Count -eq 0) { throw 'No source files found for firmware fingerprint. Build from the Git checkout.' }
    [Array]::Sort($taskPaths, [StringComparer]::Ordinal)
    $taskStream = New-Object System.IO.MemoryStream
    $taskHash = [System.Security.Cryptography.SHA256]::Create()
    try {
        foreach ($taskRelative in $taskPaths) {
            $taskNameBytes = [Text.Encoding]::UTF8.GetBytes($taskRelative + [char]0)
            $taskStream.Write($taskNameBytes, 0, $taskNameBytes.Length)
            $taskPath = Join-Path $Repository $taskRelative
            if (Test-Path -LiteralPath $taskPath -PathType Leaf) { $taskBytes = [IO.File]::ReadAllBytes($taskPath) }
            else { $taskBytes = [Text.Encoding]::UTF8.GetBytes('MISSING') }
            $taskStream.Write($taskBytes, 0, $taskBytes.Length)
            $taskStream.WriteByte(0)
        }
        return ([BitConverter]::ToString($taskHash.ComputeHash($taskStream.ToArray()))).Replace('-', '').ToLowerInvariant()
    }
    finally { $taskHash.Dispose(); $taskStream.Dispose() }
}

function Update-FlyElBuildInfo {
    param([string]$Repository)
    $taskHash = Get-FlyElGitValue $Repository @('rev-parse','HEAD')
    if ($taskHash -notmatch '^[0-9a-f]{40}$') { throw 'Missing or invalid Git revision; build metadata cannot be generated.' }
    $taskShort = Get-FlyElGitValue $Repository @('rev-parse','--short=10','HEAD')
    $taskBranch = Get-FlyElGitValue $Repository @('rev-parse','--abbrev-ref','HEAD')
    $taskTag = Get-FlyElGitValue $Repository @('describe','--tags','--exact-match','HEAD') 'NONE'
    $taskCount = Get-FlyElGitValue $Repository @('rev-list','--count','HEAD')
    $taskSubject = Get-FlyElGitValue $Repository @('show','-s','--format=%s','HEAD')
    $taskCommitDate = Get-FlyElGitValue $Repository @('show','-s','--format=%cI','HEAD')
    $taskCommitUTC = [DateTimeOffset]::Parse($taskCommitDate).UtcDateTime
    $taskDate = $taskCommitUTC.ToString('yyyy-MM-dd', [Globalization.CultureInfo]::InvariantCulture)
    $taskTime = $taskCommitUTC.ToString('HH:mm:ss', [Globalization.CultureInfo]::InvariantCulture)
    $taskStatus = Get-FlyElGitValue $Repository @('status','--porcelain','--untracked-files=normal')
    $taskTree = if ($taskStatus.Length -eq 0) { 'CLEAN' } else { 'MODIFIED' }
    $taskNow = [DateTime]::UtcNow
    $taskFields = [ordered]@{
        FW_GIT_HASH = @($taskHash, 64); FW_GIT_SHORT = @($taskShort, 12)
        FW_GIT_BRANCH = @($taskBranch, 40); FW_GIT_TAG = @($taskTag, 20)
        FW_GIT_COUNT = @($taskCount, 12); FW_GIT_SUBJECT = @($taskSubject, 200)
        FW_COMMIT_DATE = @($taskDate, 10); FW_COMMIT_TIME = @($taskTime, 8)
        FW_BUILD_DATE = @($taskNow.ToString('yyyy-MM-dd', [Globalization.CultureInfo]::InvariantCulture), 10)
        FW_BUILD_TIME = @($taskNow.ToString('HH:mm:ss', [Globalization.CultureInfo]::InvariantCulture), 8)
        FW_GIT_TREE = @($taskTree, 12); FW_SOURCE_HASH = @((Get-FlyElSourceHash $Repository), 64)
    }
    $taskLines = @('/* Generated before build; do not edit or commit. Dates are UTC. */',
        '#ifndef BUILD_INFO_GENERATED_H', '#define BUILD_INFO_GENERATED_H')
    foreach ($taskField in $taskFields.GetEnumerator()) {
        $taskLines += '#define ' + $taskField.Key + ' ' + (ConvertTo-FlyElCString $taskField.Value[0] $taskField.Value[1])
    }
    $taskLines += '#endif'
    $taskOutput = Join-Path $Repository 'build'
    New-Item -ItemType Directory -Force -Path $taskOutput | Out-Null
    [IO.File]::WriteAllText((Join-Path $taskOutput 'build_info_generated.h'), ($taskLines -join "`n") + "`n",
        (New-Object Text.UTF8Encoding($false)))
    Write-Output "Build info: $taskShort $taskTree (UTC)"
}

if ($MyInvocation.InvocationName -ne '.') {
    Update-FlyElBuildInfo $Root
    $global:LASTEXITCODE = 0
}
