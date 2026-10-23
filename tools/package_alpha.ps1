param([string]$Name='IslaViva-alpha-family-life')
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
        $runtimeAssets=@('sea-fish-v8.png','MYSTERY-V8-PROMPTS.md','ara-ara-girl.mp3','yameteh-kodasai.mp3','kyaaaaaaaa.mp3','menu-island.png','nature-reference-v5.png','buildings-reference-v5.png','civic-reference-v5.png','terrain-materials-v5.png','residents.png','residents-women-v6.png','residents-men-v6.png','residents-children-v6.png','residents-work-women-v7.png','residents-work-men-v7.png','residents-pregnancy-v7.png','boats-women-v7.png','boats-men-v7.png','boats-empty-v7.png','family-cloud-v7.png','ASSET-LICENSE.txt','PROVENANCE.md','REFERENCE-V5-PROMPTS.md','RESIDENT-V6-PROMPTS.md','ANIMATION-V7-PROMPTS.md')
        foreach($asset in $runtimeAssets){Copy-Item -LiteralPath (Join-Path $source $asset) -Destination $target -Force}
        Get-ChildItem -LiteralPath $target -File | Where-Object {$_.Name -notin $runtimeAssets} | ForEach-Object {Remove-Item -LiteralPath $_.FullName}
    }else{Get-ChildItem -LiteralPath $source -File | Copy-Item -Destination $target -Force}
}
@('@echo off','start "" "%~dp0Jugar.exe"','exit /b 0') | Set-Content -LiteralPath (Join-Path $packageRoot 'Multijugador.bat') -Encoding ASCII
foreach($file in @('ALPHA.md','MULTIPLAYER.md','TERRENO.md','PENDIENTES.md','FAMILIAS.md')){Copy-Item -LiteralPath (Join-Path $projectRoot ('native/'+$file)) -Destination $packageRoot -Force}
@('Isla Viva - Alpha de islas, poderes y visitas multijugador.','Abre Jugar.exe con doble clic. No necesita comandos de consola.','Elige tu nombre y entra a tu isla; opciones de isla contiene perfil y semilla.','Durante la partida abre o cierra tu isla, o conecta a otra. CHAT permite conversar.','Si el perfil no tiene partida se genera una isla nueva. Una semilla permite repetir su geografia.','En el juego: rueda zoom, WASD/central camara, Espacio pausa, E velocidad, Q lluvia.','Puedes encargar muelles, townhall y altares. Los NPC respetan hambre, sed y descanso nocturno.','AMPLIAR ISLA crea tierra en la costa dentro del mapa.','Alpha en desarrollo: el acabado geografico sigue en revision y no esta aprobado como reproduccion de la referencia.','Detalles: ALPHA.md, MULTIPLAYER.md, TERRENO.md y assets/PROVENANCE.md.') | Set-Content -LiteralPath (Join-Path $packageRoot 'LEEME.txt') -Encoding UTF8
if(Test-Path -LiteralPath (Join-Path $projectRoot 'native/SECURITY-REVIEW.md')){Copy-Item -LiteralPath (Join-Path $projectRoot 'native/SECURITY-REVIEW.md') -Destination (Join-Path $packageRoot 'SEGURIDAD.md') -Force}
$resolvedPackage=(Resolve-Path -LiteralPath $packageRoot).Path
$hashLines=Get-ChildItem -LiteralPath $resolvedPackage -File -Recurse | Where-Object {$_.Name -ne 'SHA256SUMS.txt'} | Sort-Object FullName | ForEach-Object {(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLower()+'  '+$_.FullName.Substring($resolvedPackage.Length+1).Replace('\','/')}
$hashLines | Set-Content -LiteralPath (Join-Path $resolvedPackage 'SHA256SUMS.txt') -Encoding ASCII
Write-Output $packageRoot
