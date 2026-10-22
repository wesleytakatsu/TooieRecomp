<#
 Windows 11 x64 build helper for TooieRecomp.  STATUS: written on a Linux host, NOT yet run on Windows.
 Stages: Check | ImportGenerated | Build | Package   (default: Check).  All sources are vendored in this repository: no bootstrap, no download.
 Generation of RecompiledFuncs (ROM -> decompressed image -> N64Recomp, kit-free: `tools/build_linux.sh preflight verify extract recomp`) needs Linux or WSL2 - see tools/BUILDING.md.
 Needs: Git for Windows (bash), Python 3.8+ (python3, python or py -3 on PATH, visible to Git Bash; used by tools/vendor_verify.sh), Visual Studio 2022 Build Tools with workload VCTools + Microsoft.VisualStudio.Component.VC.Llvm.Clang
        (clang-cl) + VC.CMake.Project (cmake/ninja) + Windows 11 SDK.  Windows PowerShell 5.1 or PowerShell 7.
 Examples:
   .\tools\windows\Build-Tooie.ps1 -Stage Check
   .\tools\windows\Build-Tooie.ps1 -Stage ImportGenerated -GeneratedFrom \\wsl$\Ubuntu-24.04\home\you\TooieRecomp\RecompiledFuncs
   .\tools\windows\Build-Tooie.ps1 -Stage Build ; .\tools\windows\Build-Tooie.ps1 -Stage Package
#>
param(
    [ValidateSet('Check','ImportGenerated','Build','Package')][string]$Stage = 'Check',
    [string]$GeneratedFrom = '',
    [int]$Jobs = 2,
    [string]$BuildDir = ''
)
$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
if (-not $BuildDir) { $BuildDir = Join-Path $Root 'build-win' }

function Find-Bash {
    $c = @("$env:ProgramFiles\Git\bin\bash.exe", "${env:ProgramFiles(x86)}\Git\bin\bash.exe") | Where-Object { Test-Path $_ }
    if (-not $c) { throw 'Git for Windows bash.exe not found (install Git for Windows).' }
    return $c[0]
}
function Invoke-Bash([string]$script) {   # run a repo shell script under Git-for-Windows bash, LF checkout required
    & (Find-Bash) -c "cd '$($Root -replace '\\','/')' && $script"
    if ($LASTEXITCODE -ne 0) { throw "bash step failed ($LASTEXITCODE): $script" }
}
function Enter-VsDev {
    $vw = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vw)) { throw 'vswhere.exe not found: install Visual Studio 2022 Build Tools.' }
    $vs = & $vw -latest -products * -requires Microsoft.VisualStudio.Component.VC.Llvm.Clang -property installationPath
    if (-not $vs) { throw 'No Visual Studio with component Microsoft.VisualStudio.Component.VC.Llvm.Clang (clang-cl).' }
    & (Join-Path $vs 'Common7\Tools\Launch-VsDevShell.ps1') -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
}

switch ($Stage) {
  'Check' {
    foreach ($t in 'git','cmake','ninja') { if (Get-Command $t -ErrorAction SilentlyContinue) { "ok   $t" } else { "MISS $t (Ninja/CMake ship with VS component VC.CMake.Project; open a VS Developer shell)" } }
    "ok   bash: $(Find-Bash)"
    Enter-VsDev; if (Get-Command clang-cl -ErrorAction SilentlyContinue) { "ok   clang-cl: $((clang-cl --version)[0])" } else { throw 'clang-cl missing' }
    if (Get-Command wsl -ErrorAction SilentlyContinue) { 'ok   wsl present (needed only for the generate stage)' } else { 'note wsl missing: generate on a Linux machine instead' }
    if ((git -C $Root config core.autocrlf) -eq 'true') { 'WARN core.autocrlf=true in this clone: re-clone with git -c core.autocrlf=false clone ... (per-clone flag; never edit global config). vendor/MANIFEST.sha256 hashes are byte-exact and need LF' }
    Invoke-Bash 'tools/vendor_verify.sh'   # offline integrity of lib/ assets/ decomp/ (needs sha256sum from Git for Windows)
    if (Test-Path (Join-Path $Root 'lib\SDL2-windows\lib\x64\SDL2.lib')) { 'ok   lib/SDL2-windows (vendored)' } else { throw 'lib/SDL2-windows missing' }
    if (Test-Path (Join-Path $Root 'RecompiledFuncs\funcs.h')) { 'ok   RecompiledFuncs present' } else { 'MISS RecompiledFuncs (generate on Linux/WSL2, then -Stage ImportGenerated)' }
  }
  'ImportGenerated' {
    if (-not (Test-Path (Join-Path $GeneratedFrom 'funcs.h'))) { throw "-GeneratedFrom must be a RecompiledFuncs directory (funcs.h not found in '$GeneratedFrom')" }
    robocopy $GeneratedFrom (Join-Path $Root 'RecompiledFuncs') /MIR /NFL /NDL /NJH /NJS | Out-Null
    if ($LASTEXITCODE -ge 8) { throw "robocopy failed ($LASTEXITCODE)" }
    "RecompiledFuncs imported ($((Get-ChildItem (Join-Path $Root 'RecompiledFuncs')).Count) files). Treat them as ROM-derived: never commit/upload."
  }
  'Build' {
    Enter-VsDev
    cmake -S $Root -B $BuildDir -G Ninja -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_BUILD_TYPE=Release
    if ($LASTEXITCODE) { throw 'cmake configure failed' }
    cmake --build $BuildDir --target TooieRecompiled -j $Jobs
    if ($LASTEXITCODE) { throw 'cmake build failed' }
  }
  'Package' {
    $exe = Join-Path $BuildDir 'TooieRecompiled.exe'; if (-not (Test-Path $exe)) { throw "missing $exe (run -Stage Build)" }
    $name = 'TooieRecompiled-windows-x64-private-local'; $out = Join-Path $Root 'dist'; $stage = Join-Path $out $name
    if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
    New-Item -ItemType Directory -Force "$stage\ui_assets","$stage\licenses" | Out-Null
    Copy-Item $exe $stage
    foreach ($d in 'SDL2.dll','dxil.dll','dxcompiler.dll') { Copy-Item (Join-Path $BuildDir $d) $stage }
    Copy-Item -Recurse (Join-Path $BuildDir 'assets') "$stage\assets"
    Copy-Item (Join-Path $Root 'ui_assets\TooieLogo.svg') "$stage\ui_assets"
    Copy-Item (Join-Path $Root 'recompcontrollerdb.txt') $stage
    # Same notice set as tools/package_linux.sh (superset of what is linked), gathered from the vendored tree (repo-relative paths).
    foreach ($d in 'vendor\licenses','lib\rt64','lib\N64ModernRuntime','lib\RecompFrontend','lib\SlotMap','lib\SDL2-windows') {
        Get-ChildItem -Path (Join-Path $Root $d) -Recurse -Depth 6 -File |
            Where-Object { $_.Name -match '^(LICENSE|LICENCE|COPYING)' } |
            ForEach-Object { $dst = Join-Path "$stage\licenses" $_.FullName.Substring($Root.Length + 1); New-Item -ItemType Directory -Force (Split-Path $dst) | Out-Null; Copy-Item $_.FullName $dst }
    }
    Copy-Item (Join-Path $Root 'lib\SDL2-windows\COPYING.txt') "$stage\licenses\SDL2-COPYING.txt"
    @('TooieRecompiled (Windows x64) - PRIVATE / LOCAL USE PACKAGE (candidate, not a committed release)','',
      'No ROM or extracted game asset inside; supply your own Banjo-Tooie (USA) rev 0 .z64 (SHA-1 af1a89e12b638b8d82cc4c085c8e01d4cba03fb3).',
      'The executable embeds machine-translated game code derived from that ROM: do not redistribute publicly before the owner decides terms.',
      'THIRD-PARTY-MANIFEST.tsv lists every file with provenance/licence status; UNKNOWN/UNVERIFIED rows are open owner decisions.',
      'Verify: from this directory, in Git-for-Windows bash:  sha256sum -c MANIFEST.sha256') | Set-Content "$stage\README-PACKAGE.txt"
    $bad = Get-ChildItem -Recurse -File $stage | Where-Object { $_.Extension -in '.z64','.n64','.v64','.elf','.sav','.eep','.log','.json' -or $_.Name -like 'baserom*' }
    if ($bad) { $bad | ForEach-Object { "AUDIT FAIL: $($_.FullName)" }; throw 'package audit failed' }
    Invoke-Bash "tools/package_manifest.sh 'dist/$name'"   # fails if any staged file lacks a rule in tools/package_licenses.tsv
    Get-ChildItem -Recurse -File $stage | Where-Object { $_.Name -ne 'MANIFEST.sha256' } | Get-FileHash -Algorithm SHA256 |
        ForEach-Object { "$($_.Hash.ToLower())  $($_.Path.Substring($stage.Length + 1).Replace('\','/'))" } | Set-Content "$stage\MANIFEST.sha256"
    Compress-Archive -Force -Path $stage -DestinationPath "$out\$name.zip"
    (Get-FileHash "$out\$name.zip" -Algorithm SHA256).Hash.ToLower() + "  $name.zip" | Set-Content "$out\$name.zip.sha256"
    "package: $out\$name.zip  (contains ROM-derived recompiled code; private/local use only; no ROM inside)"
  }
}
