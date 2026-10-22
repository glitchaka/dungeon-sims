param([string]$Name='IslaViva-alpha-wip')
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
$readme=@('Isla Viva - Alpha WIP de entorno modificable.','Extrae toda la carpeta. Abre isla_viva_native.exe y conserva assets a su lado.','Rueda: zoom. Central/WASD: mover camara. Espacio: pausa. E: velocidad. Q: lluvia.','Clic selecciona unidad; clic derecho ordena mover, cosechar, sembrar, talar o beber.','Usa los botones para encargar construccion, demolicion, reubicacion o pesca tripulada. Riesgo de perdida del 25% por viaje de pesca.','Guardar/cargar conserva la partida; tambien hay guardado automatico.','No es una version final ni una reproduccion visual identica aprobada del mapa.','Detalles: ALPHA.md. Procedencia: assets/PROVENANCE.md y cnc-license.')
$readme | Set-Content -LiteralPath (Join-Path $packageRoot 'LEEME.txt') -Encoding UTF8
Copy-Item -LiteralPath (Join-Path $projectRoot 'native/ALPHA.md') -Destination $packageRoot -Force
Write-Output $packageRoot



