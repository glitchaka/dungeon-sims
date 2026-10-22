$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot
$source=Join-Path $root 'native/assets/terrain-blocks-v3'
$catalog=Get-Content -LiteralPath (Join-Path $source 'manifest.json') -Raw | ConvertFrom-Json
if($catalog.blocks.Count -ne 24){throw 'Expected 24 modular blocks'}
Add-Type -AssemblyName System.Drawing
foreach($sheet in $catalog.sheets){
    $image=[Drawing.Bitmap]::new((Join-Path $source $sheet))
    try{foreach($block in @($catalog.blocks | Where-Object {$_.sheet -eq $sheet})){$r=$block.sourceRect;if($r.Count -ne 4 -or $r[0] -lt 0 -or $r[1] -lt 0 -or $r[2] -le 0 -or $r[3] -le 0 -or $r[0]+$r[2] -gt $image.Width -or $r[1]+$r[3] -gt $image.Height){throw "Invalid sprite rectangle: $($block.id)"}}
        $empty=0;for($y=0;$y -lt $image.Height;$y+=16){for($x=0;$x -lt $image.Width;$x+=16){if($image.GetPixel($x,$y).A -lt 2){$empty++}}};if($empty -lt 100){throw "No transparent background: $sheet"}
    }finally{$image.Dispose()}
}
$files=@(Get-ChildItem -LiteralPath $source -File -Recurse | Sort-Object FullName)
$output=Join-Path $root 'native/assets/terrain-blocks-v3.pck'
$stream=[IO.File]::Create($output)
try{$writer=[IO.BinaryWriter]::new($stream);$writer.Write([Text.Encoding]::ASCII.GetBytes('IVBLK003'));$writer.Write([uint32]$files.Count)
    foreach($file in $files){$name=[Text.Encoding]::UTF8.GetBytes($file.FullName.Substring($source.Length+1).Replace('\','/'));$data=[IO.File]::ReadAllBytes($file.FullName);$writer.Write([uint32]$name.Length);$writer.Write($name);$writer.Write([uint32]$data.Length);$writer.Write($data)};$writer.Flush()
}finally{$stream.Dispose()}
$stream=[IO.File]::OpenRead($output)
try{$reader=[IO.BinaryReader]::new($stream);if([Text.Encoding]::ASCII.GetString($reader.ReadBytes(8)) -ne 'IVBLK003'){throw 'Pack signature mismatch'};$count=$reader.ReadUInt32();if($count -ne $files.Count){throw 'Pack file count mismatch'}
    for($i=0;$i -lt $count;$i++){$name=[Text.Encoding]::UTF8.GetString($reader.ReadBytes($reader.ReadUInt32()));$data=$reader.ReadBytes($reader.ReadUInt32());$original=[IO.File]::ReadAllBytes((Join-Path $source $name));$hash=[Security.Cryptography.SHA256]::Create();try{if([Convert]::ToBase64String($hash.ComputeHash($data)) -ne [Convert]::ToBase64String($hash.ComputeHash($original))){throw "Pack changed $name"}}finally{$hash.Dispose()}}
    if($stream.Position -ne $stream.Length){throw 'Trailing pack data'}
}finally{$stream.Dispose()}
Write-Output "Verified: 24 blocks, source rectangles, transparency and $count archive entries"
