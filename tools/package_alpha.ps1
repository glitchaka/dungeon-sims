param([string]$Name='IslaViva-alpha-divine')
$ErrorActionPreference='Stop'
if($Name -notmatch '^[A-Za-z0-9_-]+$'){throw 'Invalid package name'}
$projectRoot=Split-Path $PSScriptRoot
$releaseRoot=Join-Path $projectRoot 'native/build/Release'
$packageRoot=Join-Path $projectRoot ('dist/'+$Name)
[IO.Directory]::CreateDirectory($packageRoot) | Out-Null
Copy-Item -LiteralPath (Join-Path $releaseRoot 'isla_viva_native.exe') -Destination $packageRoot -Force
Copy-Item -LiteralPath (Join-Path $releaseRoot 'cnc-import.json') -Destination $packageRoot -Force
foreach($folder in @('assets','cnc-license')){
    $target=Join-Path $packageRoot $folder
    [IO.Directory]::CreateDirectory($target) | Out-Null
    $source=if($folder -eq 'assets'){Join-Path $projectRoot 'native/assets'}else{Join-Path $releaseRoot $folder}
    Get-ChildItem -LiteralPath $source -File | Copy-Item -Destination $target -Force
}
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'run_multiplayer.ps1') -Destination $packageRoot -Force
@('@echo off','cd /d "%~dp0"','powershell -NoProfile -ExecutionPolicy Bypass -File run_multiplayer.ps1 %*','if errorlevel 1 pause') | Set-Content -LiteralPath (Join-Path $packageRoot 'Multijugador.bat') -Encoding ASCII
Copy-Item -LiteralPath (Join-Path $projectRoot 'native/MULTIPLAYER.md') -Destination $packageRoot -Force
$readme=@('Isla Viva - Alpha de autonomia, poderes divinos y visitas entre islas.','Extrae toda la carpeta. Abre isla_viva_native.exe y conserva assets a su lado.','Multijugador.bat permite abrir una isla o conectar a una IPv4. Dos ventanas locales usan perfiles separados.','Rueda: zoom. Central/WASD: mover camara. Espacio: pausa. E: velocidad. Q: lluvia.','Los habitantes deciden sus trabajos. El jugador inspecciona, inspira y envia milagros.','El elegido nace tras 270 dias del juego. Los regalos se recogen fisicamente.','Guardar/cargar conserva la partida y las visitas. Tambien hay guardado automatico.','Alpha en desarrollo. El mapa aun no reproduce de forma identica la referencia.','Detalles: ALPHA.md y MULTIPLAYER.md. Procedencia: assets/PROVENANCE.md y cnc-license.')
$readme | Set-Content -LiteralPath (Join-Path $packageRoot 'LEEME.txt') -Encoding UTF8
Copy-Item -LiteralPath (Join-Path $projectRoot 'native/ALPHA.md') -Destination $packageRoot -Force
Write-Output $packageRoot





