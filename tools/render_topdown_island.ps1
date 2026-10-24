param([int]$Seed=1749,[string]$Output='native/previews/assembled-topdown-v4.png')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot
Add-Type -AssemblyName System.Drawing
Add-Type -Path (Join-Path $PSScriptRoot 'TopDownIsland.cs') -ReferencedAssemblies System.Drawing
$layout=Get-Content (Join-Path $root 'native/assets/archive/terrain-topdown-v4/sample-layout.json') -Raw | ConvertFrom-Json
$regions=[Collections.Generic.List[double]]::new()
foreach($p in $layout.placements){foreach($v in @($p.tile,$p.x,$p.y,$p.width,$p.height)){$regions.Add([double]$v)}}
[TopDownIsland]::Render((Join-Path $root 'native/assets/archive/terrain-topdown-v4/regions.png'),(Join-Path $root $Output),$Seed,$regions.ToArray())
Get-FileHash (Join-Path $root $Output) -Algorithm SHA256 | Select-Object Hash,Path

