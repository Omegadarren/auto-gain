<#
.SYNOPSIS
    Auto Gain VST3 - automated setup and build script.
#>

$ErrorActionPreference = "Stop"
$scriptDir = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Path }

$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
               [Security.Principal.WindowsBuiltinRole]::Administrator)
if (-not $isAdmin) {
    Write-Host "  ERROR: Run as Administrator." -ForegroundColor Red
    Read-Host "`n  Press Enter to exit"; exit 1
}

function Write-Step ($msg) { Write-Host "`n>>> $msg" -ForegroundColor Cyan }
function Write-OK   ($msg) { Write-Host "    [OK] $msg" -ForegroundColor Green }
function Write-Warn ($msg) { Write-Host "    [!]  $msg" -ForegroundColor Yellow }
function Write-Fail ($msg) { Write-Host "    [ERROR] $msg" -ForegroundColor Red }

function Refresh-Path {
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" +
                [System.Environment]::GetEnvironmentVariable("Path","User")
}
function Find-Exe ($name, [string[]]$fallbackPaths) {
    Refresh-Path
    $found = Get-Command $name -ErrorAction SilentlyContinue
    if ($found) { return $found.Source }
    foreach ($p in $fallbackPaths) {
        if (Test-Path $p) { $env:Path += ";$(Split-Path $p -Parent)"; return $p }
    }
    return $null
}
function Install-Winget-Package ($id, $displayName, [string]$override = "") {
    Write-Warn "Installing $displayName via winget..."
    $args = @("install","--id",$id,"--silent","--accept-package-agreements","--accept-source-agreements")
    if ($override) { $args += @("--override", $override) }
    $proc = Start-Process -FilePath "winget" -ArgumentList $args -Wait -PassThru -NoNewWindow
    if ($proc.ExitCode -notin @(0, -1978335189)) { throw "Installation of $displayName failed." }
    Write-OK "$displayName installed"
}
function Get-VSGenerator ($installs) {
    foreach ($inst in ($installs | Sort-Object { [version]$_.installationVersion } -Descending)) {
        switch (([version]$inst.installationVersion).Major) {
            17 { return "Visual Studio 17 2022" }
            16 { return "Visual Studio 16 2019" }
            15 { return "Visual Studio 15 2017" }
        }
    }
    return $null
}

try {
    Clear-Host
    Write-Host ""
    Write-Host "  ============================================================" -ForegroundColor Cyan
    Write-Host "   AUTO GAIN VST3 - Automated Setup" -ForegroundColor White
    Write-Host "  ============================================================" -ForegroundColor Cyan
    Write-Host "  Installs Git, CMake, C++ Build Tools, then compiles the VST3."
    Write-Host "  FIRST BUILD: ~15-30 min   |   REBUILD: ~2-3 min"
    Write-Host ""
    Read-Host "  Press Enter to begin (Ctrl+C to cancel)"

    Write-Step "Step 1/5 - Checking winget..."
    if (-not (Get-Command winget -ErrorAction SilentlyContinue)) {
        Write-Fail "winget not found. Install 'App Installer' from the Microsoft Store."
        Read-Host; exit 1
    }
    Write-OK "winget available"

    Write-Step "Step 2/5 - Checking Git..."
    $gitExe = Find-Exe "git" @("C:\Program Files\Git\cmd\git.exe")
    if (-not $gitExe) {
        Install-Winget-Package "Git.Git" "Git"
        $gitExe = Find-Exe "git" @("C:\Program Files\Git\cmd\git.exe")
        if (-not $gitExe) { throw "Git not found after install. Restart Setup.bat." }
    }
    Write-OK "Git: $gitExe"

    Write-Step "Step 3/5 - Checking CMake..."
    $cmakeExe = Find-Exe "cmake" @("C:\Program Files\CMake\bin\cmake.exe")
    if (-not $cmakeExe) {
        Install-Winget-Package "Kitware.CMake" "CMake"
        $cmakeExe = Find-Exe "cmake" @("C:\Program Files\CMake\bin\cmake.exe")
        if (-not $cmakeExe) { throw "CMake not found after install. Restart Setup.bat." }
    }
    Write-OK "CMake: $cmakeExe"

    Write-Step "Step 4/5 - Checking Visual Studio C++ Build Tools..."
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $hasVCTools = $false; $vsGenerator = $null
    if (Test-Path $vswhere) {
        $raw = & $vswhere -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -format json 2>$null
        $installs = $raw | ConvertFrom-Json
        if ($installs -and $installs.Count -gt 0) {
            $hasVCTools = $true
            $vsGenerator = Get-VSGenerator $installs
            Write-OK "C++ tools found, generator: $vsGenerator"
        }
    }
    if (-not $hasVCTools) {
        Install-Winget-Package "Microsoft.VisualStudio.2022.BuildTools" "VS 2022 Build Tools" `
            "--quiet --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
        $raw = & $vswhere -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -format json 2>$null
        $installs = $raw | ConvertFrom-Json
        if ($installs -and $installs.Count -gt 0) { $hasVCTools = $true; $vsGenerator = Get-VSGenerator $installs }
        if (-not $hasVCTools) { throw "Build Tools installed but not detected. Run Setup.bat again." }
    }

    Write-Step "Step 5/5 - Building Auto Gain VST3..."
    Set-Location $scriptDir
    if (Test-Path "build\CMakeCache.txt") {
        Write-Host "  Clearing stale CMake cache..." -ForegroundColor Yellow
        Remove-Item "build" -Recurse -Force
    }
    & cmake -B build -G $vsGenerator -A x64
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed." }
    & cmake --build build --config Release --parallel
    $artefact = "build\AutoGain_artefacts\Release\VST3\Auto Gain.vst3"
    if ($LASTEXITCODE -ne 0 -and -not (Test-Path $artefact)) { throw "Build failed." }
    if (Test-Path $artefact) {
        Copy-Item -Recurse -Force $artefact "$env:CommonProgramFiles\VST3\"
    }

    Write-Host ""
    Write-Host "  ============================================================" -ForegroundColor Green
    Write-Host "   SUCCESS!  Auto Gain installed to %CommonProgramFiles%\VST3\" -ForegroundColor Green
    Write-Host "  ============================================================" -ForegroundColor Green
    Write-Host ""
    Read-Host "  Press Enter to close"

} catch {
    Write-Host "`n  SETUP FAILED: $_" -ForegroundColor Red
    Read-Host "  Press Enter to close"; exit 1
}
