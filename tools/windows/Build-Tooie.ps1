<#
 Windows 11 x64 build helper for TooieRecomp.  STATUS: Check stage validated on Windows 11 (P11 W0.6). Extract, Recomp, Configure and Build are native stages (no Linux or WSL2).
 Stages: Check | Extract | Recomp | Verify | ImportGenerated | Configure | Build | Test | Package | All   (default: Check).  -AllTests (with Test or All) also builds every other ninja executable target whose name ends in _test and runs those that take no arguments; argument-driven drivers (ArgumentDrivenTests) are built, then SKIP. Only the required host-test list fails the stage.  All sources are vendored in this repository: no bootstrap, no download.
 Extract: retail ROM -> <build>/extract/decompressed.us.z64 (tools/extract_rom.py). Recomp: build N64Recomp and regenerate RecompiledFuncs (tools/run_recomp.py), then freshness + hash verify. Verify: RecompiledFuncs vs metadata/recomp-output.sha256. ImportGenerated remains a fallback.
 Configure: Ninja + clang-cl Release into <build>, file-prefix-map=/tooie. Build: TooieRecompiled only (runs Configure when build.ninja is missing). Test: required host *_test executables (Configure when build.ninja is missing); -AllTests builds the rest and runs the ones that need no arguments. All: Check, Extract, Recomp, Configure, Build, Test, Package.
 Needs: Git for Windows (bash), Python 3.8+ (Find-Python: py -3, then python, then python3; skip WindowsApps stubs; tools/vendor_verify.sh resolves Python on its own), Visual Studio 2022 or later (verified: VS 18 / 2026)
        with VCTools + Microsoft.VisualStudio.Component.VC.Llvm.Clang (clang-cl) + VC.CMake.Project (cmake/ninja) + Windows 11 SDK.  Windows PowerShell 5.1 or PowerShell 7.
 Package: public Windows x64 archive, no ROM inside. Optional -ReleaseVersion (letters, digits, dots and dashes; no leading v); otherwise uses git describe --always --dirty.
 Examples:
   .\tools\windows\Build-Tooie.ps1 -Stage Check
   .\tools\windows\Build-Tooie.ps1 -Stage Extract
   .\tools\windows\Build-Tooie.ps1 -Stage Recomp -Jobs 2
   .\tools\windows\Build-Tooie.ps1 -Stage Verify
   .\tools\windows\Build-Tooie.ps1 -Stage ImportGenerated -GeneratedFrom \\wsl$\Ubuntu-24.04\home\you\TooieRecomp\RecompiledFuncs
   .\tools\windows\Build-Tooie.ps1 -Stage Configure
   .\tools\windows\Build-Tooie.ps1 -Stage Build ; .\tools\windows\Build-Tooie.ps1 -Stage Package
   .\tools\windows\Build-Tooie.ps1 -Stage Package -ReleaseVersion 0.1.0
   .\tools\windows\Build-Tooie.ps1 -Stage Test
   .\tools\windows\Build-Tooie.ps1 -Stage Test -AllTests
   .\tools\windows\Build-Tooie.ps1 -Stage All
#>
param(
    [ValidateSet('Check','Extract','Recomp','Verify','ImportGenerated','Configure','Build','Test','Package','All')][string]$Stage = 'Check',
    [string]$GeneratedFrom = '',
    [int]$Jobs = [Environment]::ProcessorCount,
    [string]$BuildDir = '',
    [string]$Rom = '',
    [switch]$AllTests,
    [ValidatePattern('\A(?![vV])[A-Za-z0-9.-]+\z')][string]$ReleaseVersion = ''
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
function Write-Utf8NoBom([string]$Path, [string[]]$Lines) {
    $utf8 = New-Object System.Text.UTF8Encoding $false
    [System.IO.File]::WriteAllLines($Path, $Lines, $utf8)
}
function Get-StageRelative([string]$Full, [string]$Stage) {
    $fullNorm = [System.IO.Path]::GetFullPath($Full)
    $stageNorm = [System.IO.Path]::GetFullPath($Stage).TrimEnd('\')
    $prefix = $stageNorm + '\'
    if (-not $fullNorm.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "path not under stage: $Full"
    }
    return $fullNorm.Substring($prefix.Length)
}
function Get-PackageVersion {   # same scheme as tools/package_linux.sh: git describe --always --dirty
    if (-not (Get-Command git -ErrorAction SilentlyContinue)) { return 'nogit' }
    $desc = & git -C $Root describe --always --dirty 2>$null
    if ($LASTEXITCODE -ne 0 -or -not $desc) { return 'nogit' }
    return ("$desc".Trim() -split "`n")[0].Trim()
}
function Get-ToolVersionLine([string]$Name) {
    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "$Name not found after entering the VS developer shell."
    }
    $line = @(& $Name --version 2>$null) | Select-Object -First 1
    if (-not $line) { throw "$Name --version produced no output." }
    return $line.ToString().Trim()
}
function Get-LockValue([string]$Key) {
    $lock = Join-Path $Root 'tools\toolchain.lock'
    foreach ($line in @([System.IO.File]::ReadAllLines($lock))) {
        if ($line -match ('^' + [regex]::Escape($Key) + '=(.*)$')) { return $Matches[1].Trim().Trim('"') }
    }
    throw "missing $Key in tools/toolchain.lock"
}
function Get-Sha256Hex([string]$Path) {
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLower()
}
function Add-UniqueImport($List, $Seen, [string]$Name) {
    $key = $Name.ToLowerInvariant()
    if ($Seen.ContainsKey($key)) { return }
    $Seen[$key] = $true
    [void]$List.Add($Name)
}
function Get-ImportedDlls([string]$Exe) {   # dumpbin /dependents, else llvm-readobj --coff-imports
    $names = New-Object System.Collections.Generic.List[string]
    $seen = @{}
    $dumpbin = $null
    $cmd = Get-Command dumpbin.exe -ErrorAction SilentlyContinue
    if (-not $cmd) { $cmd = Get-Command dumpbin -ErrorAction SilentlyContinue }
    if ($cmd) { $dumpbin = $cmd.Source }
    if ($dumpbin) {
        $out = @(& $dumpbin /dependents $Exe 2>$null)
        $capture = $false
        foreach ($line in $out) {
            $text = "$line"
            if ($text -match 'Image has the following (delay load )?dependencies') { $capture = $true; continue }
            if ($capture -and $text -match '^\s+Summary\s*$') { $capture = $false; continue }
            if ($capture -and $text -match '^\s+(\S+\.dll)\s*$') { Add-UniqueImport $names $seen $Matches[1] }
        }
        if ($names.Count -gt 0) {
            foreach ($n in $names) { $n }
            return
        }
    }
    $readobj = $null
    $rcmd = Get-Command llvm-readobj.exe -ErrorAction SilentlyContinue
    if (-not $rcmd) { $rcmd = Get-Command llvm-readobj -ErrorAction SilentlyContinue }
    if ($rcmd) { $readobj = $rcmd.Source }
    if (-not $readobj) {
        $clang = Get-Command clang-cl -ErrorAction SilentlyContinue
        if ($clang) {
            $sibling = Join-Path (Split-Path -Parent $clang.Source) 'llvm-readobj.exe'
            if (Test-Path -LiteralPath $sibling) { $readobj = $sibling }
        }
    }
    if (-not $readobj) { throw 'dumpbin and llvm-readobj not found after entering the VS developer shell.' }
    $out = @(& $readobj --coff-imports $Exe 2>$null)
    $readobjRc = $LASTEXITCODE
    foreach ($line in $out) {
        $text = "$line"
        if ($text -match 'DLL Name:\s*(\S+\.dll)\s*$') { Add-UniqueImport $names $seen $Matches[1] }
        elseif ($text -match '^\s+Name:\s*(\S+\.dll)\s*$') { Add-UniqueImport $names $seen $Matches[1] }
    }
    if ($names.Count -eq 0) {
        if ($readobjRc) { throw "llvm-readobj --coff-imports failed ($readobjRc)" }
        throw "no DLL imports parsed from $Exe"
    }
    foreach ($n in $names) { $n }
}
function Get-UpstreamPinLines {
    $tsv = Join-Path $Root 'vendor\UPSTREAMS.tsv'
    $rows = New-Object System.Collections.Generic.List[string]
    foreach ($line in @([System.IO.File]::ReadAllLines($tsv))) {
        if ($line -match '^\s*#' -or -not "$line".Trim()) { continue }
        $cols = "$line" -split "`t"
        if ($cols.Count -ge 3 -and $cols[0] -ne 'path') { [void]$rows.Add('  ' + $cols[0] + ' ' + $cols[2]) }
    }
    if ($rows.Count -eq 0) { throw 'vendor/UPSTREAMS.tsv has no upstream rows' }
    foreach ($row in $rows) { $row }
}
function Get-PatchShaLines {   # sha256sum-style lines, paths relative to vendor/ (Linux package_linux.sh + ORDER.tsv)
    $vendor = Join-Path $Root 'vendor'
    $files = New-Object System.Collections.Generic.List[string]
    $applied = Join-Path $vendor 'patches-applied'
    foreach ($dir in @(Get-ChildItem -LiteralPath $applied -Directory)) {
        $found = Get-ChildItem -LiteralPath $dir.FullName -File -Filter '*.patch' -ErrorAction SilentlyContinue
        if ($found) { foreach ($f in @($found)) { [void]$files.Add($f.FullName) } }
    }
    $localDir = Join-Path $vendor 'patches-local'
    $local = Get-ChildItem -LiteralPath $localDir -File -Filter '*.patch' -ErrorAction SilentlyContinue
    if ($local) { foreach ($f in @($local)) { [void]$files.Add($f.FullName) } }
    $order = Join-Path $applied 'ORDER.tsv'
    if (-not (Test-Path -LiteralPath $order)) { throw "missing $order" }
    [void]$files.Add($order)
    if ($files.Count -lt 2) { throw 'no patches-applied / patches-local files to hash' }
    $sorted = New-Object System.Collections.Generic.List[string]
    foreach ($full in $files) { [void]$sorted.Add($full) }
    $sorted.Sort([System.StringComparer]::Ordinal)
    $lines = New-Object System.Collections.Generic.List[string]
    foreach ($full in $sorted) {
        $rel = (Get-StageRelative $full $vendor).Replace('\', '/')
        [void]$lines.Add((Get-Sha256Hex $full) + '  ' + $rel)
    }
    foreach ($line in $lines) { $line }
}
function Test-PackageForbidden([System.IO.FileInfo]$File, [string]$Relative) {
    $ext = $File.Extension.ToLowerInvariant()
    if ($ext -in '.z64', '.n64', '.v64', '.elf', '.sav', '.eep', '.log') { return $true }
    if ($File.Name -like 'baserom*' -or $File.Name -like 'decompressed*') { return $true }
    $atRoot = ($Relative.IndexOf('\') -lt 0) -and ($Relative.IndexOf('/') -lt 0)
    if ($atRoot -and $ext -eq '.json') { return $true }
    foreach ($cfg in @('graphics.json', 'general.json', 'sound.json', 'controls.json')) {
        if ($File.Name -eq $cfg) { return $true }
    }
    return $false
}
function Invoke-Package {
    Enter-VsDev
    $exe = Join-Path $BuildDir 'TooieRecompiled.exe'
    if (-not (Test-Path -LiteralPath $exe)) { throw "missing $exe (run -Stage Build)" }
    $ver = Get-PackageVersion
    $releaseLabel = 'none'
    $buildLabel = "dev build $ver"
    $name = "BanjoTooieRecompiled-dev-$ver-Windows-x64"
    if ($ReleaseVersion) {
        $releaseLabel = $ReleaseVersion
        $buildLabel = "v$ReleaseVersion"
        $name = "BanjoTooieRecompiled-v$ReleaseVersion-Windows-x64"
    }
    $out = Join-Path $Root 'dist'
    $stage = Join-Path (Join-Path $out $name) 'BanjoTooieRecompiled'
    if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
    New-Item -ItemType Directory -Force "$stage\ui_assets", "$stage\licenses" | Out-Null
    Copy-Item -LiteralPath $exe -Destination $stage
    foreach ($d in 'SDL2.dll', 'dxil.dll', 'dxcompiler.dll') {
        $src = Join-Path $BuildDir $d
        if (-not (Test-Path -LiteralPath $src)) { throw "missing $src" }
        Copy-Item -LiteralPath $src -Destination $stage
    }
    $assets = Join-Path $BuildDir 'assets'
    if (-not (Test-Path -LiteralPath $assets)) { throw "missing $assets" }
    Copy-Item -Recurse -LiteralPath $assets -Destination "$stage\assets"
    Copy-Item -LiteralPath (Join-Path $Root 'ui_assets\TooieLogo.svg') -Destination "$stage\ui_assets"
    Copy-Item -LiteralPath (Join-Path $Root 'recompcontrollerdb.txt') -Destination $stage
    # Same notice set as tools/package_linux.sh (superset of what is linked), gathered from the vendored tree (repo-relative paths).
    foreach ($d in 'vendor\licenses', 'lib\rt64', 'lib\N64ModernRuntime', 'lib\RecompFrontend', 'lib\SlotMap', 'lib\SDL2-windows') {
        Get-ChildItem -Path (Join-Path $Root $d) -Recurse -Depth 6 -File |
            Where-Object { $_.Name -match '^(LICENSE|LICENCE|COPYING)' } |
            ForEach-Object { $dst = Join-Path "$stage\licenses" $_.FullName.Substring($Root.Length + 1); New-Item -ItemType Directory -Force (Split-Path $dst) | Out-Null; Copy-Item $_.FullName $dst }
    }
    Copy-Item (Join-Path $Root 'lib\SDL2-windows\COPYING.txt') "$stage\licenses\SDL2-COPYING.txt"
    $stagedExe = Join-Path $stage 'TooieRecompiled.exe'
    $imports = @(Get-ImportedDlls $stagedExe)
    $bundled = @{ 'sdl2.dll' = $true; 'dxil.dll' = $true; 'dxcompiler.dll' = $true }
    $dep = New-Object System.Collections.Generic.List[string]
    [void]$dep.Add('# Runtime dependencies of TooieRecompiled.exe (SYSTEM DLLs are NOT bundled).')
    [void]$dep.Add('# Direct imports: ' + ($imports -join ' '))
    [void]$dep.Add('# columns: dll<TAB>disposition<TAB>note')
    foreach ($dll in $imports) {
        if ($bundled.ContainsKey($dll.ToLowerInvariant())) {
            [void]$dep.Add("$dll`tBUNDLED`tshipped next to TooieRecompiled.exe")
        } else {
            [void]$dep.Add("$dll`tSYSTEM (Windows)`tnot bundled")
        }
    }
    Write-Utf8NoBom (Join-Path $stage 'DEPENDENCIES.txt') ([string[]]$dep.ToArray())
    $commit = 'unknown'
    $dirtyLabel = 'unavailable (source export without Git metadata)'
    if (Get-Command git -ErrorAction SilentlyContinue) {
        $head = & git -C $Root rev-parse HEAD 2>$null
        if ($LASTEXITCODE -eq 0 -and $head) { $commit = ("$head".Trim() -split "`n")[0].Trim() }
        $status = & git -C $Root status --short 2>$null
        if ($LASTEXITCODE -eq 0) {
            if (-not $status) { $dirtyLabel = '0' } else { $dirtyLabel = "$($(@($status).Count))" }
        }
    }
    $info = New-Object System.Collections.Generic.List[string]
    [void]$info.Add('TooieRecompiled build info')
    [void]$info.Add("tooie_commit: $commit")
    [void]$info.Add("tooie_tree_dirty_files: $dirtyLabel")
    [void]$info.Add("release_version: $releaseLabel")
    [void]$info.Add('ROM_SHA1: ' + (Get-LockValue 'ROM_SHA1'))
    [void]$info.Add('vendored_upstreams (path, pin sha; full table in vendor/UPSTREAMS.tsv):')
    foreach ($row in @(Get-UpstreamPinLines)) { [void]$info.Add($row) }
    [void]$info.Add('vendor_manifest_sha256: ' + (Get-Sha256Hex (Join-Path $Root 'vendor\MANIFEST.sha256')))
    [void]$info.Add('patches_applied_sha256 (already integrated in lib/; provenance only):')
    foreach ($row in @(Get-PatchShaLines)) { [void]$info.Add($row) }
    [void]$info.Add('compiler: ' + (Get-ToolVersionLine 'clang-cl'))
    [void]$info.Add('cmake: ' + (Get-ToolVersionLine 'cmake'))
    [void]$info.Add('binary_sha256: ' + (Get-Sha256Hex $stagedExe))
    $utc = [DateTime]::UtcNow.ToString('yyyy-MM-ddTHH:mm:ss', [Globalization.CultureInfo]::InvariantCulture) + 'Z'
    [void]$info.Add("build_time_utc: $utc")
    Write-Utf8NoBom (Join-Path $stage 'BUILD-INFO.txt') ([string[]]$info.ToArray())
    $readme = @(
        "Banjo-Tooie Recompiled $buildLabel (Windows x64)",
        '',
        'A native PC port of Banjo-Tooie made with static recompilation (N64Recomp).',
        'Contains no game assets and no ROM: graphics, audio and other game data are read from your own ROM at run time.',
        '',
        'Requirements: your own Banjo-Tooie (USA) rev 0 cartridge dump',
        '(SHA-1 af1a89e12b638b8d82cc4c085c8e01d4cba03fb3, .z64 or .n64).',
        'The launcher accepts that retail cart (it decompresses internally) or the decompressed image',
        '(35098224 bytes, SHA-1 af479771ff5e91ce3732f3d84146614ab71d8d79).',
        'GPU: Direct3D 12.',
        '',
        'How to play: double-click TooieRecompiled.exe and select your ROM in the launcher.',
        'Data: user config and saves go to %LOCALAPPDATA%\TooieRecompiled, never into this directory.',
        '',
        'Banjo-Tooie is (c) Nintendo / Rare. This is an unofficial fan project, not affiliated with or endorsed by',
        'Nintendo or Rare. The executable contains code translated from the game; use a ROM dumped from a cartridge you own.',
        'Third-party components and their licences: licenses/ and THIRD-PARTY-MANIFEST.tsv.',
        '',
        'Verify: from this directory, in Git for Windows bash: sha256sum -c MANIFEST.sha256',
        'Source: https://github.com/wesleytakatsu/TooieRecomp'
    )
    Write-Utf8NoBom (Join-Path $stage 'README-PACKAGE.txt') $readme
    $hits = New-Object System.Collections.Generic.List[string]
    $scanned = 0
    foreach ($f in @(Get-ChildItem -LiteralPath $stage -Recurse -File)) {
        $scanned++
        $rel = (Get-StageRelative $f.FullName $stage).Replace('\', '/')
        if (Test-PackageForbidden $f $rel) { [void]$hits.Add($rel) }
    }
    $audit = New-Object System.Collections.Generic.List[string]
    [void]$audit.Add('audit: checked=no .z64/.n64/.v64/.elf/.sav/.eep/.log; no baserom*; no decompressed*; no package-root *.json; no user config graphics.json, general.json, sound.json, controls.json anywhere in the tree')
    [void]$audit.Add("audit: files_scanned=$scanned")
    if ($hits.Count -eq 0) {
        [void]$audit.Add('audit: forbidden=none')
        [void]$audit.Add('audit: result=PASS')
    } else {
        [void]$audit.Add('audit: forbidden=FOUND')
        foreach ($hit in $hits) { [void]$audit.Add("audit: hit=$hit") }
        [void]$audit.Add('audit: result=FAIL')
    }
    Write-Utf8NoBom (Join-Path $stage 'AUDIT.txt') ([string[]]$audit.ToArray())
    foreach ($line in $audit) { Write-Host $line }
    if ($hits.Count -gt 0) {
        foreach ($hit in $hits) { Write-Host "AUDIT FAIL: $hit" }
        throw 'package audit failed'
    }
    Invoke-Bash "tools/package_manifest.sh 'dist/$name/BanjoTooieRecompiled'"   # fails if any staged file lacks a rule in tools/package_licenses.tsv
    $rels = New-Object System.Collections.Generic.List[string]
    foreach ($f in @(Get-ChildItem -LiteralPath $stage -Recurse -File)) {
        $rel = (Get-StageRelative $f.FullName $stage).Replace('\', '/')
        if ($rel -ne 'MANIFEST.sha256') { [void]$rels.Add($rel) }
    }
    $rels.Sort([System.StringComparer]::Ordinal)
    $manifest = New-Object System.Collections.Generic.List[string]
    foreach ($rel in $rels) {
        $full = Join-Path $stage ($rel.Replace('/', '\'))
        [void]$manifest.Add((Get-Sha256Hex $full) + '  ' + $rel)
    }
    Write-Utf8NoBom (Join-Path $stage 'MANIFEST.sha256') ([string[]]$manifest.ToArray())
    $zip = Join-Path $out "$name.zip"
    Compress-Archive -Force -Path $stage -DestinationPath $zip
    $zipHash = (Get-Sha256Hex $zip) + '  ' + "$name.zip"
    [System.IO.File]::WriteAllText("$zip.sha256", $zipHash + "`n", (New-Object System.Text.UTF8Encoding $false))
    Invoke-Py @('tools/verify_package.py', $zip)   # non-zero fails the stage
    "package: $zip  (contains ROM-derived recompiled code; no ROM inside)"
}
$script:RequiredTests = @(
    'tooie_language_pack_test',
    'tooie_text_font_map_test',
    'tooie_language_import_test',
    'tooie_font_sprite_test',
    'tooie_language_bundle_test',
    'tooie_menu_policy_test',
    'tooie_menu_config_test',
    'rt64_host_endpoint_current_test',
    'p09_interp_burst_test',
    'p09_world_interpolation_guard_test',
    'p09_local_pair_snapshot_test',
    'p09_exact_use_correlation_test',
    'rt64_present_decimator_test',
    'rom_cart_import_test'
)
# Bare invocation prints a usage line and exits non-zero. Built under -AllTests; not run.
# tools/build_linux.sh does not run this set either.
$script:ArgumentDrivenTests = @{
    'tooie_demo1_parity_test'           = 'hook.h path'
    'tooie_demo1_projectile_test'       = 'hook.h path'
    'tooie_demo1_target_selector_test'  = 'cur.csv orc.csv'
    'tooie_demo1_upd_layout_test'       = 'current.csv oracle.csv'
    'p09_local_pair_snapshot_io_test'   = '--write-fixture / --replay-file style multi-process invocation'
}

function Get-DiscoveredTestTargets {   # ninja -t targets all: "<name>: phony" or "<name>.exe: ...EXECUTABLE..."
    $lines = @(& ninja -C $BuildDir -t targets all)
    if ($LASTEXITCODE) { throw "ninja -t targets failed ($LASTEXITCODE)" }
    $phony = New-Object System.Collections.Generic.List[string]
    $exes = New-Object System.Collections.Generic.List[string]
    $seenPhony = @{}
    $seenExe = @{}
    foreach ($line in $lines) {
        $text = "$line".Trim()
        if ($text -notmatch '^([^:\s]+):\s+(\S+)') { continue }
        $name = $Matches[1]
        $rule = $Matches[2]
        if ($name -match '[\\/]') { continue }
        if (($rule -eq 'phony') -and $name.EndsWith('_test')) {
            if (-not $seenPhony.ContainsKey($name)) { $seenPhony[$name] = $true; $phony.Add($name) }
        } elseif (($rule -like '*EXECUTABLE*') -and $name.EndsWith('_test.exe')) {
            $stem = $name.Substring(0, $name.Length - 4)
            if (-not $seenExe.ContainsKey($stem)) { $seenExe[$stem] = $true; $exes.Add($stem) }
        }
    }
    if ($exes.Count -eq 0) { return @($phony) }
    $phonySet = @{}
    foreach ($n in $phony) { $phonySet[$n] = $true }
    $picked = New-Object System.Collections.Generic.List[string]
    foreach ($n in $exes) {
        if (($phonySet.Count -eq 0) -or $phonySet.ContainsKey($n)) { $picked.Add($n) }
    }
    return @($picked)
}

function Invoke-HostTest([string]$Name) {   # cwd = build dir; stdout+stderr -> <BuildDir>\<name>.log
    $exe = Join-Path $BuildDir ($Name + '.exe')
    $log = Join-Path $BuildDir ($Name + '.log')
    if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) {
        [System.IO.File]::WriteAllText($log, "missing executable: $exe`r`n")
        Write-Host "FAIL $Name (see $log)"
        return $false
    }
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $exe
    $psi.WorkingDirectory = $BuildDir
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.CreateNoWindow = $true
    $proc = New-Object System.Diagnostics.Process
    $proc.StartInfo = $psi
    [void]$proc.Start()
    $stdout = $proc.StandardOutput.ReadToEndAsync()
    $stderr = $proc.StandardError.ReadToEndAsync()
    [void]$proc.WaitForExit()
    $stdout.Wait()
    $stderr.Wait()
    $text = [string]$stdout.Result
    $errText = [string]$stderr.Result
    if ($errText) {
        if ($text -and -not $text.EndsWith("`n")) { $text += "`r`n" }
        $text += $errText
    }
    [System.IO.File]::WriteAllText($log, $text)
    if ($proc.ExitCode -eq 0) {
        Write-Host "PASS $Name"
        return $true
    }
    Write-Host "FAIL $Name (see $log)"
    return $false
}

function Invoke-Test {
    Enter-VsDev
    if (-not (Test-Path (Join-Path $BuildDir 'build.ninja'))) { Invoke-Configure }
    $cmakeArgs = @('--build', $BuildDir)
    foreach ($t in $script:RequiredTests) { $cmakeArgs += @('--target', $t) }
    $cmakeArgs += @('-j', "$Jobs")
    & cmake @cmakeArgs
    if ($LASTEXITCODE) { throw 'cmake build of required tests failed' }
    $reqPass = 0
    foreach ($t in $script:RequiredTests) {
        if (Invoke-HostTest $t) { $reqPass++ }
    }
    $extraPass = 0
    $extraTotal = 0
    $skipped = 0
    if ($AllTests) {
        $requiredSet = @{}
        foreach ($t in $script:RequiredTests) { $requiredSet[$t] = $true }
        foreach ($t in @(Get-DiscoveredTestTargets)) {
            if ($requiredSet.ContainsKey($t)) { continue }
            $needsArgs = $script:ArgumentDrivenTests.ContainsKey($t)
            if (-not $needsArgs) { $extraTotal++ }
            & cmake --build $BuildDir --target $t -j $Jobs
            if ($LASTEXITCODE) {
                Write-Host "BUILD-FAIL $t"
                continue
            }
            if ($needsArgs) {
                Write-Host "SKIP $t (requires arguments: $($script:ArgumentDrivenTests[$t]))"
                $skipped++
                continue
            }
            if (Invoke-HostTest $t) { $extraPass++ }
        }
    }
    Write-Host "tests: required $reqPass/$($script:RequiredTests.Count) PASS; extra $extraPass/$extraTotal PASS; skipped $skipped"
    if ($reqPass -ne $script:RequiredTests.Count) { throw 'required tests failed' }
}

function Invoke-All {
    Invoke-Check
    Invoke-Extract
    Invoke-Recomp
    Invoke-Configure
    Invoke-Build
    Invoke-Test
    Invoke-Package
}

switch ($Stage) {
  'Check' { Invoke-Check }
  'Extract' { Invoke-Extract }
  'Recomp' { Invoke-Recomp }
  'Verify' { Invoke-Verify }
  'ImportGenerated' { Invoke-ImportGenerated }
  'Configure' { Invoke-Configure }
  'Build' { Invoke-Build }
  'Test' { Invoke-Test }
  'Package' { Invoke-Package }
  'All' { Invoke-All }
}
