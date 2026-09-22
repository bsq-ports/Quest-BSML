param([switch]$Run, [string]$Device, [string]$Adb = "adb")
$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "../..")).Path
Push-Location $repoRoot
try {
    $taskNdk = (Get-Content -LiteralPath "ndkpath.txt" -Raw).Trim()
    $compiler = Join-Path $taskNdk "toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe"
    New-Item -ItemType Directory -Force -Path "build/quest-probe" | Out-Null
    & $compiler --target=aarch64-linux-android24 -std=c++26 -Wno-c23-extensions -O1 -static-libstdc++ -fPIE -pie `
        -Itests/quest -Isrc/BSML/Animations/GIF -Iinclude/submodules/EasyGifReader `
        -Iextern/includes/gif-lib/shared tests/quest/decoder-probe.cpp `
        include/submodules/EasyGifReader/EasyGifReader.cpp extern/libs/libgif-lib.debug.so `
        -o build/quest-probe/gif-decoder-probe
    if ($LASTEXITCODE -ne 0) { throw "Decoder probe build failed" }
    if ($Run) {
        $deviceArgs = @()
        if ($Device) { $deviceArgs = @("-s", $Device) }
        & $Adb @deviceArgs push build/quest-probe/gif-decoder-probe /data/local/tmp/bsml-gif-decoder-probe
        if ($LASTEXITCODE -ne 0) { throw "Probe upload failed" }
        & $Adb @deviceArgs shell chmod 700 /data/local/tmp/bsml-gif-decoder-probe
        if ($LASTEXITCODE -ne 0) { throw "Probe chmod failed" }
        & $Adb @deviceArgs shell timeout 30 /data/local/tmp/bsml-gif-decoder-probe
        if ($LASTEXITCODE -ne 0) { throw "Decoder regression failed or exceeded 30 seconds" }
    }
} finally { Pop-Location }

