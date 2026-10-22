param([switch]$BuildOnly, [switch]$SmokeTest)
$ErrorActionPreference='Stop'
$projectRoot=Split-Path $PSScriptRoot
$buildRoot=Join-Path $projectRoot 'native/build'
try {
    if (!(Get-Command cmake -ErrorAction SilentlyContinue)) { throw 'Se necesita CMake y Visual Studio 2022 con C++ para compilar.' }
    & cmake -S (Join-Path $projectRoot 'native') -B $buildRoot -G 'Visual Studio 17 2022' -A x64
    if($LASTEXITCODE -ne 0) { throw 'No se pudo configurar el proyecto nativo.' }
    & cmake --build $buildRoot --config Release --parallel
    if($LASTEXITCODE -ne 0) { throw 'Fallo la compilacion nativa.' }
    if(!$BuildOnly) {
        $gameArguments = @()
        if ($SmokeTest) { $gameArguments += '--smoke-test' }
        & (Join-Path $buildRoot 'Release/isla_viva_native.exe') @gameArguments
        if($LASTEXITCODE -ne 0) { throw 'El juego nativo termino con un error.' }
    }
} catch { Write-Error $_; exit 1 }
