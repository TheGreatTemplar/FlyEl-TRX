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
Write-Output 'PASS build metadata: escaping, bounds, current revision, stable SHA256'
