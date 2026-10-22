<#
 Windows 11 x64 build helper for TooieRecomp.  STATUS: Check stage validated on Windows 11 (P11 W0.6). Extract, Recomp, Configure and Build are native stages (no Linux or WSL2).
 Stages: Check | Extract | Recomp | Verify | ImportGenerated | Configure | Build | Package | All   (default: Check).  All sources are vendored in this repository: no bootstrap, no download.
 Extract: retail ROM -> <build>/extract/decompressed.us.z64 (tools/extract_rom.py). Recomp: build N64Recomp and regenerate RecompiledFuncs (tools/run_recomp.py), then freshness + hash verify. Verify: RecompiledFuncs vs metadata/recomp-output.sha256. ImportGenerated remains a fallback.
 Configure: Ninja + clang-cl Release into <build>, file-prefix-map=/tooie. Build: TooieRecompiled only (runs Configure when build.ninja is missing). All: Check, Extract, Recomp, Configure, Build.
 Needs: Git for Windows (bash), Python 3.8+ (Find-Python: py -3, then python, then python3; skip WindowsApps stubs; tools/vendor_verify.sh resolves Python on its own), Visual Studio 2022 or later (verified: VS 18 / 2026)
        with VCTools + Microsoft.VisualStudio.Component.VC.Llvm.Clang (clang-cl) + VC.CMake.Project (cmake/ninja) + Windows 11 SDK.  Windows PowerShell 5.1 or PowerShell 7.
 Examples:
   .\tools\windows\Build-Tooie.ps1 -Stage Check
   .\tools\windows\Build-Tooie.ps1 -Stage Extract
   .\tools\windows\Build-Tooie.ps1 -Stage Recomp -Jobs 2
   .\tools\windows\Build-Tooie.ps1 -Stage Verify
   .\tools\windows\Build-Tooie.ps1 -Stage ImportGenerated -GeneratedFrom \\wsl$\Ubuntu-24.04\home\you\TooieRecomp\RecompiledFuncs
   .\tools\windows\Build-Tooie.ps1 -Stage Configure
   .\tools\windows\Build-Tooie.ps1 -Stage Build ; .\tools\windows\Build-Tooie.ps1 -Stage Package
   .\tools\windows\Build-Tooie.ps1 -Stage All
#>
param(
    [ValidateSet('Check','Extract','Recomp','Verify','ImportGenerated','Configure','Build','Package','All')][string]$Stage = 'Check',
    [string]$GeneratedFrom = '',
    [int]$Jobs = [Environment]::ProcessorCount,
    [string]$BuildDir = '',
    [string]$Rom = ''
)
$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
# Python stages read TOOIE_BUILD_DIR. Set it only when -BuildDir was actually passed, so the scripts share that tree.
$useBuildDir = $false
if ($PSBoundParameters.ContainsKey('BuildDir') -and $BuildDir) { $useBuildDir = $true }
if (-not $BuildDir) { $BuildDir = Join-Path $Root 'build-win' }
if ($useBuildDir) { $env:TOOIE_BUILD_DIR = $BuildDir }

function Find-Bash {
    $c = @(@("$env:ProgramFiles\Git\bin\bash.exe", "${env:ProgramFiles(x86)}\Git\bin\bash.exe") | Where-Object { Test-Path $_ })
    if (-not $c) { throw 'Git for Windows bash.exe not found (install Git for Windows).' }
    return $c[0]
}
function Invoke-Bash([string]$script) {   # run a repo shell script under Git-for-Windows bash, LF checkout required
    & (Find-Bash) -c "cd '$($Root -replace '\\','/')' && $script"
    if ($LASTEXITCODE -ne 0) { throw "bash step failed ($LASTEXITCODE): $script" }
}
function Find-Python {   # command array: @('py','-3') or @('C:\Python313\python.exe'); skip Store stubs; need >= 3.8
    foreach ($spec in 'py -3', 'python', 'python3') {
        $parts = @($spec -split ' ')
        $exe = $parts[0]
        $rest = @()
        if ($parts.Count -gt 1) { $rest = @($parts[1..($parts.Count - 1)]) }
        $gc = Get-Command $exe -ErrorAction SilentlyContinue
        if (-not $gc) { continue }
        $src = $gc.Source
        if ($src -like '*\WindowsApps\*') { continue }
        try {
            $ver = & $exe @rest -c "import sys; print('%d.%d.%d' % sys.version_info[:3])" 2>$null
        } catch { continue }
        if ($LASTEXITCODE -ne 0) { continue }
        $verText = "$(@($ver) | Select-Object -Last 1)".Trim()
        if ($verText -notmatch '^(\d+)\.(\d+)\.') { continue }
        $maj = [int]$Matches[1]; $min = [int]$Matches[2]
        if (($maj -lt 3) -or (($maj -eq 3) -and ($min -lt 8))) { continue }
        if ($rest.Count -eq 0) { return ,@($src) }
        return ,(@($exe) + @($rest))
    }
    throw 'Python 3.8+ not found (tried py -3, python, python3; skipped WindowsApps stubs).'
}
function Invoke-Py([string[]]$ScriptAndArgs) {   # Find-Python + args, cwd = repo root; non-zero exit throws
    $py = @(Find-Python)
    if ($py.Count -eq 1 -and $py[0] -is [System.Array]) { $py = @($py[0]) }
    $rest = @()
    if ($py.Count -gt 1) { $rest = @($py[1..($py.Count - 1)]) }
    Push-Location $Root
    try {
        & $py[0] @rest @ScriptAndArgs
        if ($LASTEXITCODE -ne 0) { throw "python step failed ($LASTEXITCODE): $($ScriptAndArgs -join ' ')" }
    } finally {
        Pop-Location
    }
}
function Enter-VsDev {   # idempotent: clang-cl already on PATH means the developer shell is active
    if (Get-Command clang-cl -ErrorAction SilentlyContinue) { return }
    $vw = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vw)) { throw 'vswhere.exe not found: install Visual Studio 2022 or later (Community or Build Tools) with clang-cl.' }
    $vsPath = @(& $vw -latest -products * -requires Microsoft.VisualStudio.Component.VC.Llvm.Clang -property installationPath) | Where-Object { $_ -and "$_".Trim() } | Select-Object -First 1
    if (-not $vsPath) { throw 'No Visual Studio 2022 or later with Microsoft.VisualStudio.Component.VC.Llvm.Clang (clang-cl).' }
    # VsDevCmd.bat invokes vswhere.exe by bare name; the Installer dir is not on PATH by default.
    $installerDir = Split-Path -Parent $vw
    $installerNorm = $installerDir.TrimEnd('\')
    $onPath = $false
    foreach ($entry in @($env:PATH -split ';')) {
        if ($entry.Trim().TrimEnd('\') -eq $installerNorm) { $onPath = $true; break }
    }
    if (-not $onPath) { $env:PATH = "$installerDir;$env:PATH" }
    & (Join-Path "$vsPath".Trim() 'Common7\Tools\Launch-VsDevShell.ps1') -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
}
function Show-Tool([string]$Name) {   # first stdout line of --version; cmake/ninja/clang-cl must exist after Enter-VsDev
    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        if ($Name -eq 'git') { throw 'git not found (install Git for Windows).' }
        throw "$Name not found after entering the VS developer shell."
    }
    $line = @(& $Name --version 2>$null) | Select-Object -First 1
    if (-not $line) { throw "$Name --version produced no output." }
    "ok   ${Name}: $($line.ToString().Trim())"
}

function Invoke-Check {
    Enter-VsDev
    foreach ($t in 'git','cmake','ninja','clang-cl') { Show-Tool $t }
    $py = @(Find-Python)
    if ($py.Count -eq 1 -and $py[0] -is [System.Array]) { $py = @($py[0]) }
    $rest = @()
    if ($py.Count -gt 1) { $rest = @($py[1..($py.Count - 1)]) }
    $pyInfo = @(& $py[0] @rest -c "import sys; print(sys.executable); print('%d.%d.%d' % sys.version_info[:3])")
    "ok   python: $($pyInfo[0].ToString().Trim()) $($pyInfo[1].ToString().Trim())"
    "ok   bash: $(Find-Bash)"
    if (Get-Command wsl -ErrorAction SilentlyContinue) { 'note wsl: not needed (native pipeline)' }
    $crlf = @(git -C $Root ls-files --eol | Where-Object { $_ -match '^i/lf\s+w/crlf' })
    if ($crlf.Count -gt 0) { "WARN $($crlf.Count) files are LF in git but CRLF on disk: re-clone with git -c core.autocrlf=false clone ..." } else { 'ok   line endings (LF)' }
    Invoke-Bash 'tools/vendor_verify.sh'   # offline integrity of lib/ assets/ decomp/ (needs sha256sum from Git for Windows)
    if (Test-Path (Join-Path $Root 'lib\SDL2-windows\lib\x64\SDL2.lib')) { 'ok   lib/SDL2-windows (vendored)' } else { throw 'lib/SDL2-windows missing' }
    if (Test-Path (Join-Path $Root 'RecompiledFuncs\funcs.h')) { 'ok   RecompiledFuncs present' } else { 'MISS RecompiledFuncs (native Extract/Recomp stages: P11 W1; fallback: -Stage ImportGenerated)' }
}
function Invoke-Extract {
    Enter-VsDev
    $pyArgs = @('tools/extract_rom.py')
    if ($Rom) { $pyArgs += $Rom }
    Invoke-Py $pyArgs
}
function Invoke-Recomp {
    Enter-VsDev
    Invoke-Py @('tools/run_recomp.py', '--jobs', "$Jobs")
    Invoke-Py @('tools/check_generated_fresh.py')
    Invoke-Py @('tools/verify_generated.py')
}
function Invoke-Verify {
    Invoke-Py @('tools/verify_generated.py')
}
function Invoke-ImportGenerated {
    if (-not (Test-Path (Join-Path $GeneratedFrom 'funcs.h'))) { throw "-GeneratedFrom must be a RecompiledFuncs directory (funcs.h not found in '$GeneratedFrom')" }
    robocopy $GeneratedFrom (Join-Path $Root 'RecompiledFuncs') /MIR /NFL /NDL /NJH /NJS | Out-Null
    if ($LASTEXITCODE -ge 8) { throw "robocopy failed ($LASTEXITCODE)" }
    "RecompiledFuncs imported ($((Get-ChildItem (Join-Path $Root 'RecompiledFuncs')).Count) files). Treat them as ROM-derived: never commit/upload."
}
function Invoke-Configure {   # clang-cl defaults live in CMAKE_*_FLAGS because -D replaces them; prefix-map via /clang:
    Enter-VsDev
    $rootFwd = $Root -replace '\\', '/'
    $pm = "/clang:-ffile-prefix-map=${rootFwd}=/tooie"
    $cFlags = "/DWIN32 /D_WINDOWS $pm"
    $cxxFlags = "/DWIN32 /D_WINDOWS /EHsc $pm"
    # One argv each: PS 5.1/7 re-quote on whitespace, so a Root with spaces stays inside the -D value.
    cmake -S $Root -B $BuildDir -G Ninja -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_BUILD_TYPE=Release "-DCMAKE_C_FLAGS=$cFlags" "-DCMAKE_CXX_FLAGS=$cxxFlags"
    if ($LASTEXITCODE) { throw 'cmake configure failed' }
}
function Invoke-Build {
    Enter-VsDev
    if (-not (Test-Path (Join-Path $BuildDir 'build.ninja'))) { Invoke-Configure }
    cmake --build $BuildDir --target TooieRecompiled -j $Jobs
    if ($LASTEXITCODE) { throw 'cmake build failed' }
}
function Invoke-Package {
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
function Invoke-All {
    Invoke-Check
    Invoke-Extract
    Invoke-Recomp
    Invoke-Configure
    Invoke-Build
    # Test and Package will be appended later.
}

switch ($Stage) {
  'Check' { Invoke-Check }
  'Extract' { Invoke-Extract }
  'Recomp' { Invoke-Recomp }
  'Verify' { Invoke-Verify }
  'ImportGenerated' { Invoke-ImportGenerated }
  'Configure' { Invoke-Configure }
  'Build' { Invoke-Build }
  'Package' { Invoke-Package }
  'All' { Invoke-All }
}
