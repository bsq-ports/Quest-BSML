param(
    [string]$Unity = 'C:/Program Files/Unity/Hub/Editor/6000.0.40f1/Editor/Unity.exe'
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$log = Join-Path $repo 'build/shader-validation.log'
New-Item -ItemType Directory -Force (Split-Path $log) | Out-Null
$process = Start-Process -FilePath $Unity -WindowStyle Hidden -Wait -PassThru -ArgumentList @(
    '-batchmode', '-quit', '-force-d3d11',
    '-projectPath', ('"' + (Join-Path $repo 'shaders') + '"'),
    '-executeMethod', 'ValidateIndexedGif.RunBoth', '-logFile', ('"' + $log + '"')
)
if ($process.ExitCode -ne 0) { throw "Shader validation failed; see $log" }
Write-Output "Shader GPU validation passed; see $log"
