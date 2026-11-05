# Cross-compiles the handheld build (TK64_MOBILE) for aarch64 Linux from Windows. Run inside PowerShell 7; opens the VS developer environment itself.
#   tools\xu10\build-xu10.ps1 [-Configure] [-Target TooieRecompiled] [-Jobs N] [-Sysroot <dir>] [-FileToC <file_to_c.exe>]
# Sysroot: from tools/xu10/fetch_sysroot.py, or $env:XU10_SYSROOT. FileToC: built for Windows, by default from build-win (Build-Tooie.ps1 -Stage Build).
param([switch]$Configure, [string]$Target = "TooieRecompiled", [int]$Jobs = 0, [string]$Sysroot = $env:XU10_SYSROOT, [string]$FileToC = "")
$ErrorActionPreference = "Stop"
$root = Resolve-Path "$PSScriptRoot\..\.."
$vw = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = @(& $vw -latest -products * -requires Microsoft.VisualStudio.Component.VC.Llvm.Clang -property installationPath) | Select-Object -First 1
& (Join-Path $vs 'Common7\Tools\Launch-VsDevShell.ps1') -Arch amd64 -HostArch amd64 -SkipAutomaticLocation | Out-Null
if (-not $Sysroot) { throw "Pass -Sysroot or set XU10_SYSROOT (tools/xu10/fetch_sysroot.py makes one)." }
$env:XU10_SYSROOT = $Sysroot.Replace('\', '/')
$build = Join-Path $root "build-xu10"
if ($Configure -or -not (Test-Path (Join-Path $build "build.ninja"))) {
    if (-not $FileToC) { $FileToC = Join-Path $root "build-win\rt64\src\tools\file_to_c\file_to_c.exe" }
    if (-not (Test-Path $FileToC)) { throw "file_to_c not found: $FileToC" }
    $fileToC = $FileToC.Replace('\', '/')
    $pkg = (Join-Path $PSScriptRoot "pkg-config.cmd").Replace('\', '/')
    & cmake -S $root -B $build -G Ninja `
        "-DCMAKE_TOOLCHAIN_FILE=$($PSScriptRoot.Replace('\', '/'))/aarch64-rocknix.cmake" `
        -DCMAKE_BUILD_TYPE=Release `
        -DTK64_MOBILE=ON `
        "-DTK64_HOST_FILE_TO_C=$fileToC" `
        "-DPKG_CONFIG_EXECUTABLE=$pkg" `
        -DNFD_PORTAL=ON `
        -DCMAKE_CXX_STANDARD_LIBRARIES=-ldbus-1
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
$jobArgs = @(); if ($Jobs -gt 0) { $jobArgs = @("-j", "$Jobs") }
& ninja -C $build @jobArgs -k 0 $Target
exit $LASTEXITCODE
