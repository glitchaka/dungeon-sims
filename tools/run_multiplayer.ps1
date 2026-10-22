param([string]$IP,[switch]$HostIsland,[string]$Profile,[int]$Port=32145)
$ErrorActionPreference='Stop'
# Works both in the repository and in the extracted portable package.
$gameRoot=if(Test-Path (Join-Path $PSScriptRoot 'isla_viva_native.exe')){$PSScriptRoot}else{Join-Path (Split-Path $PSScriptRoot) 'dist/IslaViva-alpha-divine'}
$gameExe=Join-Path $gameRoot 'isla_viva_native.exe'
if(-not(Test-Path -LiteralPath $gameExe)){throw 'Falta la alpha multijugador. Compila o extrae el paquete completo.'}
if(-not $HostIsland -and -not $IP){
    Write-Host '1: Abrir mi isla para recibir otro jugador'
    Write-Host '2: Conectar con la isla de otro jugador'
    $choice=Read-Host 'Elige 1 o 2'
    if($choice -eq '1'){$HostIsland=$true}elseif($choice -eq '2'){$IP=Read-Host 'IPv4 del otro jugador (127.0.0.1 para otra ventana en este PC)'}else{throw 'Opcion invalida'}
}
if($HostIsland){if(-not $Profile){$Profile='island-host'};& $gameExe --host --port $Port --profile $Profile}
else{
    $address=[Net.IPAddress]::Parse($IP)
    if($address.AddressFamily -ne [Net.Sockets.AddressFamily]::InterNetwork){throw 'Usa una direccion IPv4'}
    if(-not $Profile){$Profile='island-guest'}
    & $gameExe --join $address.IPAddressToString --port $Port --profile $Profile
}
exit $LASTEXITCODE
