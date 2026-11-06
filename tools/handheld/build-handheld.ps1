# Cross-compiles the handheld build (CMake preset linux-arm64-handheld, TK64_MOBILE) for aarch64 Linux from Windows.
# Run inside PowerShell 7; opens the Visual Studio developer environment itself. On a Linux host use build-handheld.sh.
#   tools\handheld\build-handheld.ps1 [-Configure] [-Target TooieRecompiled[,other...]] [-Jobs N] [-Sysroot <dir>] [-Device rk3326] [-FileToC <file_to_c.exe>]
# Sysroot: from tools/handheld/fetch_sysroot.py, or $env:HANDHELD_SYSROOT. FileToC: built for Windows, by default from build-win
# (Build-Tooie.ps1 -Stage Build). Device: a file in tools/handheld/devices.
param([switch]$Configure, [string[]]$Target = @("TooieRecompiled"), [int]$Jobs = 0, [string]$Sysroot = "", [string]$Device = "",
      [string]$FileToC = "")
$ErrorActionPreference = "Stop"
$root = (Resolve-Path "$PSScriptRoot\..\..").Path
if (-not $Sysroot) { $Sysroot = if ($env:HANDHELD_SYSROOT) { $env:HANDHELD_SYSROOT } else { $env:XU10_SYSROOT } }
if (-not $Sysroot) { throw "Pass -Sysroot or set HANDHELD_SYSROOT (tools/handheld/fetch_sysroot.py makes one)." }
$vw = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = @(& $vw -latest -products * -requires Microsoft.VisualStudio.Component.VC.Llvm.Clang -property installationPath) | Select-Object -First 1
if (-not $vs) { throw "Visual Studio with the Clang component (Microsoft.VisualStudio.Component.VC.Llvm.Clang) not found." }
& (Join-Path $vs 'Common7\Tools\Launch-VsDevShell.ps1') -Arch amd64 -HostArch amd64 -SkipAutomaticLocation | Out-Null
$env:HANDHELD_SYSROOT = $Sysroot.Replace('\', '/')
$env:HANDHELD_LLVM = (Join-Path $vs 'VC\Tools\Llvm\x64\bin').Replace('\', '/')
$env:HANDHELD_PKG_CONFIG = (Join-Path $PSScriptRoot "pkg-config.cmd").Replace('\', '/')
$build = Join-Path $root "build-handheld"
if ($Configure -or -not (Test-Path (Join-Path $build "build.ninja"))) {
    if (-not $FileToC) { $FileToC = Join-Path $root "build-win\rt64\src\tools\file_to_c\file_to_c.exe" }
    if (-not (Test-Path $FileToC)) { throw "file_to_c not found: $FileToC (build Windows first: tools\windows\Build-Tooie.ps1 -Stage Build)" }
    $env:TK64_HOST_FILE_TO_C = (Resolve-Path $FileToC).Path.Replace('\', '/')
    $presetArgs = @("--preset", "linux-arm64-handheld")
    if ($Device) { $presetArgs += "-DHANDHELD_DEVICE=$Device" }
    Push-Location $root
    try { & cmake @presetArgs } finally { Pop-Location }
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
$jobArgs = @(); if ($Jobs -gt 0) { $jobArgs = @("-j", "$Jobs") }
# pwsh -File passes -Target a,b as one string.
$Target = @($Target | ForEach-Object { $_ -split ',' } | Where-Object { $_ })
& ninja -C $build @jobArgs -k 0 @Target
exit $LASTEXITCODE
