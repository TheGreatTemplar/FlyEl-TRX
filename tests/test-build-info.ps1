$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $taskRoot 'scripts/update-build-info.ps1')
if ((ConvertTo-FlyElCString 'a\b"c') -cne '"a\\b\"c"') { throw 'C string escaping failed' }
if ((ConvertTo-FlyElCString ('A' * 50) 20).Length -ne 22) { throw 'Bounded strings failed' }
if ((ConvertTo-FlyElCString "line`nnext") -cne '"line next"') { throw 'Whitespace normalization failed' }
$taskBefore = Get-FlyElSourceHash $taskRoot
Update-FlyElBuildInfo $taskRoot
$taskAfter = Get-FlyElSourceHash $taskRoot
if ($taskBefore -cne $taskAfter -or $taskAfter -notmatch '^[0-9a-f]{64}$') { throw 'Source fingerprint is not stable' }
$taskHeader = Get-Content -Raw -LiteralPath (Join-Path $taskRoot 'build/build_info_generated.h')
$taskRevision = Get-FlyElGitValue $taskRoot @('rev-parse','HEAD')
if (-not $taskHeader.Contains('"' + $taskRevision + '"')) { throw 'Generated revision does not match Git' }
if (-not $taskHeader.Contains('"' + $taskAfter + '"')) { throw 'Generated fingerprint does not match sources' }
$taskSavedPath = $env:PATH
$taskSavedGit = $env:FLYEL_GIT
try {
    $taskResolved = Resolve-FlyElGit
    $env:PATH = ''
    $env:FLYEL_GIT = $taskSavedGit
    # Reproduces an IDE launched before Codex added its bundled Git to PATH.
    if ((Get-FlyElGitValue $taskRoot @('rev-parse','HEAD')) -cne $taskRevision) { throw 'Git discovery without PATH failed' }
    Update-FlyElBuildInfo $taskRoot
    $taskNoPathHeader = Get-Content -Raw -LiteralPath (Join-Path $taskRoot 'build/build_info_generated.h')
    if ($taskNoPathHeader -match '#define FW_(GIT_HASH|GIT_SHORT|GIT_COUNT|COMMIT_DATE|COMMIT_TIME|SOURCE_HASH) "UNKNOWN"') {
        throw 'No-PATH build silently contains UNKNOWN metadata'
    }
    $env:FLYEL_GIT = $taskResolved
    if ((Resolve-FlyElGit) -cne $taskResolved) { throw 'Explicit Git executable override failed' }
    $env:FLYEL_GIT = Join-Path $taskRoot 'build/missing-git.exe'
    $taskRejected = $false
    try { Update-FlyElBuildInfo $taskRoot } catch { $taskRejected = $true }
    if (-not $taskRejected) { throw 'Missing Git was silently accepted' }
    $env:FLYEL_GIT = $taskResolved
    $taskRejected = $false
    try { Get-FlyElGitValue $taskRoot @('rev-parse','this-ref-does-not-exist') | Out-Null } catch { $taskRejected = $true }
    if (-not $taskRejected) { throw 'Required Git failures were silently accepted' }
}
finally { $env:PATH = $taskSavedPath; $env:FLYEL_GIT = $taskSavedGit }
Write-Output 'PASS build metadata: escaping, stable SHA256, Git without PATH, explicit path and hard failures'
