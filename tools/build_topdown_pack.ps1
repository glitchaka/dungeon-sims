param()
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot
$folder=Join-Path $root 'native/assets/terrain-topdown-v4'
Add-Type -AssemblyName System.Drawing
$image=[Drawing.Image]::FromFile((Join-Path $folder 'regions.png'))
try {$w=$image.Width;$h=$image.Height} finally {$image.Dispose()}
$names=@('meadow-stones','meadow-dry','rolling-hills','forest-broadleaf','forest-mixed','rocky-hills','massif-a','massif-b','alpine-lake')
$regions=for($i=0;$i -lt 9;$i++){[ordered]@{id=$names[$i];tile=$i;sourceRect=@(([int]($i%3)*[int]($w/3)),([int][Math]::Floor($i/3)*[int]($h/3)),([int]($w/3)),([int]($h/3)))}}
$manifest=[ordered]@{format='IslaViva Terrain Pack';version=4;magic='IVTPACK4';camera='nadir_90_degrees';columns=3;rows=3;atlas='regions.png';regions=@($regions);runtimeForest='independent mutable props; forest cells are preview-only';sourceSHA256=(Get-FileHash (Join-Path $folder 'regions.png')).Hash}
$json=$manifest | ConvertTo-Json -Depth 8
$json | Set-Content (Join-Path $folder 'manifest.json') -Encoding UTF8
$meta=[Text.Encoding]::UTF8.GetBytes($json);$png=[IO.File]::ReadAllBytes((Join-Path $folder 'regions.png'))
$out=Join-Path $root 'native/assets/terrain-topdown-v4.pck'
$stream=[IO.File]::Create($out);$writer=[IO.BinaryWriter]::new($stream)
try {$writer.Write([Text.Encoding]::ASCII.GetBytes('IVTPACK4'));$writer.Write([uint32]$meta.Length);$writer.Write([uint32]$png.Length);$writer.Write($meta);$writer.Write($png)} finally {$writer.Dispose()}
$reader=[IO.BinaryReader]::new([IO.File]::OpenRead($out))
try {
 if([Text.Encoding]::ASCII.GetString($reader.ReadBytes(8)) -ne 'IVTPACK4'){throw 'Magic mismatch'}
 $ml=$reader.ReadUInt32();$pl=$reader.ReadUInt32();$reader.ReadBytes($ml) | Out-Null
 $payload=$reader.ReadBytes($pl);$hash=[Security.Cryptography.SHA256]::Create();try {$actual=([BitConverter]::ToString($hash.ComputeHash($payload))).Replace('-','')} finally {$hash.Dispose()}
 if($actual -ne $manifest.sourceSHA256){throw 'PNG roundtrip mismatch'}
 if($reader.BaseStream.Position -ne $reader.BaseStream.Length){throw 'Unexpected trailing data'}
} finally {$reader.Dispose()}
Write-Output "IVTPACK4: 9 regions; PNG hash verified; $out"
