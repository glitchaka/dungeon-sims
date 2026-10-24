$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot
$manifest=[IO.File]::ReadAllText((Join-Path $root 'native/assets/archive/terrain-pack/manifest.json'))
$metadata=[Text.Encoding]::UTF8.GetBytes($manifest)
$png=[IO.File]::ReadAllBytes((Join-Path $root 'native/assets/archive/terrain-pack/terrain-atlas.png'))
$stream=[IO.File]::Create((Join-Path $root 'native/assets/archive/terrain-islands.pck'))
try{$writer=[IO.BinaryWriter]::new($stream);$writer.Write([Text.Encoding]::ASCII.GetBytes('IVTPACK2'));$writer.Write([uint32]$metadata.Length);$writer.Write([uint32]$png.Length);$writer.Write($metadata);$writer.Write($png);$relief=[IO.File]::ReadAllBytes((Join-Path $root 'native/assets/archive/terrain-pack/natural-relief.png'));$writer.Write([uint32]$relief.Length);$writer.Write($relief);$writer.Flush()}finally{$stream.Dispose()}


