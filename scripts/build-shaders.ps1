param(
    [string]$Unity = 'C:/Program Files/Unity/Hub/Editor/6000.0.40f1/Editor/Unity.exe'
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
if (-not (Test-Path -LiteralPath $Unity)) { throw "Unity Editor not found: $Unity" }
$project = Join-Path $repo 'shaders'
$log = Join-Path $repo 'build/shaders-build.log'
New-Item -ItemType Directory -Force (Split-Path $log) | Out-Null
$process = Start-Process -FilePath $Unity -WindowStyle Hidden -Wait -PassThru -ArgumentList @(
    '-batchmode', '-nographics', '-quit', '-buildTarget', 'Android',
    '-projectPath', ('"' + $project + '"'),
    '-executeMethod', 'BuildIndexedGif.Build', '-logFile', ('"' + $log + '"')
)
if ($process.ExitCode -ne 0) { throw "Shader build failed; see $log" }
Write-Output "Built assets/shaders/bsml-indexed-gif.bundle"
