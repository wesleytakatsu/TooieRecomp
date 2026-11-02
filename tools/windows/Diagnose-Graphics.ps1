param(
    [string]$Executable = (Join-Path $PSScriptRoot '..\..\build-win\TooieRecompiled.exe'),
    [string]$OutputDirectory = (Join-Path $PSScriptRoot '..\..\build-win\graphics-diagnostics')
)
$ErrorActionPreference = 'Stop'
$Executable = (Resolve-Path -LiteralPath $Executable).Path
$session = Join-Path $OutputDirectory (Get-Date -Format 'yyyyMMdd-HHmmss-fff')
New-Item -ItemType Directory -Path $session -Force | Out-Null
$session = (Resolve-Path -LiteralPath $session).Path
$previousLog = [Environment]::GetEnvironmentVariable('TOOIE_GRAPHICS_LOG', 'Process')
$previousAuto = [Environment]::GetEnvironmentVariable('TOOIE_AUTO_START', 'Process')
$previousSmoke = [Environment]::GetEnvironmentVariable('TOOIE_LAUNCHER_MUSIC_SMOKE', 'Process')
try {
    $env:TOOIE_GRAPHICS_LOG = Join-Path $session 'graphics.jsonl'
    Remove-Item Env:TOOIE_AUTO_START -ErrorAction SilentlyContinue
    Remove-Item Env:TOOIE_LAUNCHER_MUSIC_SMOKE -ErrorAction SilentlyContinue
    Write-Host "Graphics log: $env:TOOIE_GRAPHICS_LOG"
    Write-Host 'Use the launcher normally, then close the game to finish the session.'
    $process = Start-Process -FilePath $Executable -WorkingDirectory (Split-Path -Parent $Executable) -PassThru -Wait
    Write-Host "Exit code: $($process.ExitCode)"
    if (Test-Path -LiteralPath $env:TOOIE_GRAPHICS_LOG) {
        Get-Content -LiteralPath $env:TOOIE_GRAPHICS_LOG |
            ForEach-Object { $_ | ConvertFrom-Json } |
            Where-Object event -In @('selected', 'attempt_failed', 'game_start') |
            Format-Table event, api, device, code -AutoSize
    }
} finally {
    [Environment]::SetEnvironmentVariable('TOOIE_GRAPHICS_LOG', $previousLog, 'Process')
    [Environment]::SetEnvironmentVariable('TOOIE_AUTO_START', $previousAuto, 'Process')
    [Environment]::SetEnvironmentVariable('TOOIE_LAUNCHER_MUSIC_SMOKE', $previousSmoke, 'Process')
}
