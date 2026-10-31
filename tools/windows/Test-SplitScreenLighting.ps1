<#
.SYNOPSIS
Run the current Windows build with lightweight split-screen lighting logs.
.DESCRIPTION
Backs up saves and JSON configuration, records build identity, then captures
stdout, stderr and model-allocation lighting events. Does not enable Dev Mode
or the experimental two-player option; enable them normally in the game.
Close the game window to finish capture. Do not use Save and Quit for this test.
.EXAMPLE
pwsh -NoProfile -File .\tools\windows\Test-SplitScreenLighting.ps1
.EXAMPLE
pwsh -NoProfile -File .\tools\windows\Test-SplitScreenLighting.ps1 -DisablePrivateVertices
#>
param(
    [string]$LogDir = '',
    [switch]$DisablePrivateVertices
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$exePath = Join-Path $repoRoot 'build-win\TooieRecompiled.exe'
if (-not (Test-Path -LiteralPath $exePath -PathType Leaf)) {
    throw "Build the game first: tools\windows\Build-Tooie.ps1 -Stage Build"
}
if (Get-Process -Name TooieRecompiled,TooieRecompiled_dbg -ErrorAction SilentlyContinue) {
    throw 'Close the running TooieRecompiled game before starting a separate log capture.'
}
if (-not $LogDir) {
    $LogDir = Join-Path $env:TEMP ('TooieRecomp-lighting-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
}
$LogDir = [IO.Path]::GetFullPath($LogDir)
if (Test-Path -LiteralPath $LogDir) {
    throw "Use a new log directory to preserve earlier captures: $LogDir"
}
New-Item -ItemType Directory -Path $LogDir | Out-Null
$backupDir = Join-Path $LogDir 'backup'
New-Item -ItemType Directory -Path $backupDir | Out-Null
$appDataDir = Join-Path $env:LOCALAPPDATA 'TooieRecompiled'
if (Test-Path -LiteralPath $appDataDir -PathType Container) {
    Get-ChildItem -LiteralPath $appDataDir -File |
        Where-Object { $_.Name -match '\.json(\.bak)?$' } |
        Copy-Item -Destination $backupDir
    $savesDir = Join-Path $appDataDir 'saves'
    if (Test-Path -LiteralPath $savesDir -PathType Container) {
        Copy-Item -LiteralPath $savesDir -Destination (Join-Path $backupDir 'saves') -Recurse
    }
}

$head = (& git -C $repoRoot rev-parse HEAD) -join ''
if ($LASTEXITCODE -ne 0) { throw 'Cannot read the source revision.' }
$dirty = @(& git -C $repoRoot status --short)
if ($LASTEXITCODE -ne 0) { throw 'Cannot read the worktree status.' }
$runInfo = [ordered]@{
    StartedUtc = [DateTime]::UtcNow.ToString('o')
    StartedLocal = (Get-Date).ToString('o')
    Head = $head
    Worktree = $dirty
    Executable = $exePath
    ExecutableSha256 = (Get-FileHash -LiteralPath $exePath -Algorithm SHA256).Hash
    PrivateVertices = -not $DisablePrivateVertices.IsPresent
    LogDirectory = $LogDir
    ProcessId = $null
    EndedUtc = $null
    ExitCode = $null
}
$metadataPath = Join-Path $LogDir 'run.json'
function Write-RunInfo {
    [IO.File]::WriteAllText($metadataPath, ($runInfo | ConvertTo-Json -Depth 5), [Text.UTF8Encoding]::new($false))
}
Write-RunInfo

$captureEnv = @{
    TOOIE_AUTO_START = '1'
    TOOIE_DIAG_2P_LIGHTING_LOG = (Join-Path $LogDir 'lighting.log')
    TOOIE_TEST_2P_PRIVATE_VERTICES = $(if ($DisablePrivateVertices) { '0' } else { '1' })
}
$previousEnv = @{}
foreach ($name in $captureEnv.Keys) {
    $previousEnv[$name] = [Environment]::GetEnvironmentVariable($name, 'Process')
}
try {
    foreach ($name in $captureEnv.Keys) {
        [Environment]::SetEnvironmentVariable($name, $captureEnv[$name], 'Process')
    }
    Write-Host "Logs and backup: $LogDir"
    Write-Host "Private lighting vertices: $($runInfo.PrivateVertices)"
    Write-Host 'Unlock Dev Mode and enable Two players (split screen), then load the story map.'
    Write-Host 'Capture video normally. Close the game window to finish; do not use Save and Quit.'
    $game = Start-Process -FilePath $exePath -WorkingDirectory (Split-Path -Parent $exePath) -PassThru `
        -RedirectStandardOutput (Join-Path $LogDir 'stdout.log') `
        -RedirectStandardError (Join-Path $LogDir 'stderr.log')
    $runInfo.ProcessId = $game.Id
    Write-RunInfo
    $game.WaitForExit()
    $game.Refresh()
    $runInfo.ExitCode = $game.ExitCode
} finally {
    $runInfo.EndedUtc = [DateTime]::UtcNow.ToString('o')
    foreach ($name in $captureEnv.Keys) {
        [Environment]::SetEnvironmentVariable($name, $previousEnv[$name], 'Process')
    }
    Write-RunInfo
}
Write-Host "Capture finished. Logs: $LogDir"
