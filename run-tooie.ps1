# Launch TooieRecompiled.exe (RT64). PowerShell 5.1 and 7.
# Equivalent of run-tooie.sh. No param() block: `--` must reach $args.
$ErrorActionPreference = 'Stop'

$Here = $PSScriptRoot
if (-not $Here) { $Here = Split-Path -Parent $MyInvocation.MyCommand.Path }
$BuildDir = if ($env:TOOIE_BUILD_DIR) { $env:TOOIE_BUILD_DIR } else { Join-Path $Here 'build-win' }
$Bin = Join-Path $BuildDir 'TooieRecompiled.exe'

function Show-Usage {
    @'
Usage: .\run-tooie.ps1 [options] [-- extra args for TooieRecompiled]

Starts TooieRecompiled with RT64. This is the TooieRecomp port, not the
Angrylion oracle. For Mupen + Angrylion RDP Plus + CXD4 LLE (Linux):

  ./run-tooie-angrylion-rdp.sh

Options:
  -h, --help            Show this help
  -a, --auto-start      Start game immediately (TOOIE_AUTO_START=1)
  -A, --attract         Auto-start + attract diagnostics
                        (TOOIE_AUTO_START=1 TOOIE_DIAG_ATTRACT=1)
  -d, --diag            Enable diagnostics (TOOIE_DIAG=1)
  -m, --launcher-music  Allow hidden launcher music preview (default: off)
  -n, --no-auto         Stay on launcher (default)

Environment (optional overrides):
  TOOIE_BUILD_DIR, TOOIE_AUTO_START, TOOIE_DIAG, TOOIE_DIAG_ATTRACT,
  TOOIE_DISABLE_LAUNCHER_MUSIC, TOOIE_DIAG_INTERVAL_MS

Examples:
  .\run-tooie.ps1                 # launcher UI
  .\run-tooie.ps1 -a              # boot straight into game
  .\run-tooie.ps1 -A              # attract/demo capture (P08)
  .\run-tooie.ps1 -a -d           # auto-start + diag
'@
}

function Set-EnvDefault([string]$Name, [string]$Value) {
    $current = [Environment]::GetEnvironmentVariable($Name, 'Process')
    if ([string]::IsNullOrEmpty($current)) {
        Set-Item -Path "Env:$Name" -Value $Value
    }
}

# CommandLineToArgvW quoting so PowerShell 5.1 and 7 pass extra args intact.
function ConvertTo-WinArg([string]$Arg) {
    if ($Arg -notmatch '[\s"]') { return $Arg }
    $sb = New-Object System.Text.StringBuilder
    [void]$sb.Append('"')
    $bs = 0
    foreach ($ch in $Arg.ToCharArray()) {
        if ($ch -eq '\') { $bs++; continue }
        if ($ch -eq '"') {
            if ($bs -gt 0) { [void]$sb.Append('\', ($bs * 2)) }
            [void]$sb.Append('\"')
            $bs = 0
            continue
        }
        if ($bs -gt 0) { [void]$sb.Append('\', $bs); $bs = 0 }
        [void]$sb.Append($ch)
    }
    if ($bs -gt 0) { [void]$sb.Append('\', ($bs * 2)) }
    [void]$sb.Append('"')
    return $sb.ToString()
}

$auto = 0
$diag = 0
$attract = 0
$launcherMusic = 0
$extra = New-Object System.Collections.Generic.List[string]
$i = 0
while ($i -lt $args.Count) {
    $a = [string]$args[$i]
    if ($a -ceq '-h' -or $a -ceq '--help') { Show-Usage; exit 0 }
    elseif ($a -ceq '-a' -or $a -ceq '--auto-start') { $auto = 1 }
    elseif ($a -ceq '-A' -or $a -ceq '--attract') { $auto = 1; $attract = 1 }
    elseif ($a -ceq '-d' -or $a -ceq '--diag') { $diag = 1 }
    elseif ($a -ceq '-m' -or $a -ceq '--launcher-music') { $launcherMusic = 1 }
    elseif ($a -ceq '-n' -or $a -ceq '--no-auto') { $auto = 0 }
    elseif ($a -ceq '--') {
        $i++
        while ($i -lt $args.Count) { $extra.Add([string]$args[$i]); $i++ }
        break
    }
    else { $extra.Add($a) }
    $i++
}

if (-not (Test-Path -LiteralPath $Bin -PathType Leaf)) {
    [Console]::Error.WriteLine("error: executable not found: $Bin")
    [Console]::Error.WriteLine('Build first, e.g.: .\tools\windows\Build-Tooie.ps1 -Stage Build')
    exit 1
}

Set-EnvDefault 'TOOIE_AUTO_START' "$auto"
Set-EnvDefault 'TOOIE_DIAG' "$diag"
if ($attract -eq 1) { Set-EnvDefault 'TOOIE_DIAG_ATTRACT' '1' }
if ($launcherMusic -eq 1) {
    Remove-Item Env:\TOOIE_DISABLE_LAUNCHER_MUSIC -ErrorAction SilentlyContinue
    $env:TOOIE_DISABLE_LAUNCHER_MUSIC = '0'
} else {
    Set-EnvDefault 'TOOIE_DISABLE_LAUNCHER_MUSIC' '1'
}

$attractShown = $env:TOOIE_DIAG_ATTRACT
if ([string]::IsNullOrEmpty($attractShown)) { $attractShown = '0' }

Set-Location -LiteralPath $BuildDir
Write-Output "[run-tooie] $Bin  (RT64)"
Write-Output "[run-tooie] AUTO_START=$env:TOOIE_AUTO_START DIAG=$env:TOOIE_DIAG DIAG_ATTRACT=$attractShown DISABLE_LAUNCHER_MUSIC=$env:TOOIE_DISABLE_LAUNCHER_MUSIC"

$argLine = (@($extra) | ForEach-Object { ConvertTo-WinArg $_ }) -join ' '
$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = $Bin
$psi.WorkingDirectory = $BuildDir
$psi.UseShellExecute = $false
$psi.Arguments = $argLine
$proc = [System.Diagnostics.Process]::Start($psi)
if (-not $proc) { exit 1 }
$proc.WaitForExit()
exit $proc.ExitCode
