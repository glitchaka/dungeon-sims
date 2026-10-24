param([string]$IP,[switch]$HostIsland,[string]$Profile='default',[int]$Port=32145)
$ErrorActionPreference='Stop'
$gameRoot=if(Test-Path (Join-Path $PSScriptRoot 'isla_viva_native.exe')){$PSScriptRoot}else{Join-Path (Split-Path $PSScriptRoot) 'dist/IslaViva-alpha-islands'}
$game=Join-Path $gameRoot 'isla_viva_native.exe'
if(-not(Test-Path -LiteralPath $game)){throw 'Falta el paquete completo de la alpha'}
$arguments=@('--profile',$Profile,'--port',"$Port")
if($HostIsland){$arguments+='--host'}
if($IP){$arguments+=@('--join',$IP)}
Start-Process -FilePath $game -ArgumentList $arguments -WindowStyle Hidden

