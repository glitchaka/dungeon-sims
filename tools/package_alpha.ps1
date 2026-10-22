param([string]$Name='IslaViva-alpha-islands')
$ErrorActionPreference='Stop'
if($Name -notmatch '^[A-Za-z0-9_-]+$'){throw 'Invalid package name'}
$projectRoot=Split-Path $PSScriptRoot
$releaseRoot=Join-Path $projectRoot 'native/build/Release'
$packageRoot=Join-Path $projectRoot ('dist/'+$Name)
[IO.Directory]::CreateDirectory($packageRoot) | Out-Null
foreach($file in @('isla_viva_native.exe','Jugar.exe','cnc-import.json')){Copy-Item -LiteralPath (Join-Path $releaseRoot $file) -Destination $packageRoot -Force}
foreach($folder in @('assets','cnc-license')){
    $target=Join-Path $packageRoot $folder
    [IO.Directory]::CreateDirectory($target) | Out-Null
    $source=if($folder -eq 'assets'){Join-Path $projectRoot 'native/assets'}else{Join-Path $releaseRoot $folder}
    if($folder -eq 'assets'){
        $runtimeAssets=@('building-pieces.png','civic-buildings.png','civic-pieces.json','fields.png','house.png','nature-pieces.png','objects.png','pieces.json','residents.png','road-tiles.png','terrain-topdown-v4.pck','ASSET-LICENSE.txt','PROVENANCE.md')
        foreach($asset in $runtimeAssets){Copy-Item -LiteralPath (Join-Path $source $asset) -Destination $target -Force}
        Get-ChildItem -LiteralPath $target -File | Where-Object {$_.Name -notin $runtimeAssets} | ForEach-Object {Remove-Item -LiteralPath $_.FullName}
    }else{Get-ChildItem -LiteralPath $source -File | Copy-Item -Destination $target -Force}
}
@('@echo off','start "" "%~dp0Jugar.exe"','exit /b 0') | Set-Content -LiteralPath (Join-Path $packageRoot 'Multijugador.bat') -Encoding ASCII
foreach($file in @('ALPHA.md','MULTIPLAYER.md','TERRENO.md','PENDIENTES.md')){Copy-Item -LiteralPath (Join-Path $projectRoot ('native/'+$file)) -Destination $packageRoot -Force}
@('Isla Viva - Alpha de islas, poderes y visitas multijugador.','Abre Jugar.exe con doble clic. No necesita comandos de consola.','Elige nombre, perfil de guardado y modo: individual, abrir partida o conectar a la direccion de otro jugador.','La pantalla muestra la direccion local del anfitrion y permite copiarla. CHAT permite conversar.','Si el perfil no tiene partida se genera una isla nueva. Una semilla permite repetir su geografia.','En el juego: rueda zoom, WASD/central camara, Espacio pausa, E velocidad, Q lluvia.','Puedes encargar muelles, townhall y altares. Los NPC respetan hambre, sed y descanso nocturno.','AMPLIAR ISLA crea tierra en la costa dentro del mapa.','Alpha en desarrollo: el acabado geografico sigue en revision y no esta aprobado como reproduccion de la referencia.','Detalles: ALPHA.md, MULTIPLAYER.md, TERRENO.md y assets/PROVENANCE.md.') | Set-Content -LiteralPath (Join-Path $packageRoot 'LEEME.txt') -Encoding UTF8
if(Test-Path -LiteralPath (Join-Path $projectRoot 'native/SECURITY-REVIEW.md')){Copy-Item -LiteralPath (Join-Path $projectRoot 'native/SECURITY-REVIEW.md') -Destination (Join-Path $packageRoot 'SEGURIDAD.md') -Force}
$resolvedPackage=(Resolve-Path -LiteralPath $packageRoot).Path
$hashLines=Get-ChildItem -LiteralPath $resolvedPackage -File -Recurse | Where-Object {$_.Name -ne 'SHA256SUMS.txt'} | Sort-Object FullName | ForEach-Object {(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLower()+'  '+$_.FullName.Substring($resolvedPackage.Length+1).Replace('\','/')}
$hashLines | Set-Content -LiteralPath (Join-Path $resolvedPackage 'SHA256SUMS.txt') -Encoding ASCII
Write-Output $packageRoot
