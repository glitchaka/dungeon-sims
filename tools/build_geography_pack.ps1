param([string]$Source='native/assets/terrain-geography-v2',[string]$Output='native/assets/terrain-geography-v2.pck')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot
$sourcePath=Join-Path $root $Source
$files=@(Get-ChildItem -LiteralPath $sourcePath -File | Sort-Object Name)
foreach($required in @('mountain-region.png','coastal-region.png','upland-region.png','manifest.json')){if(-not(Test-Path -LiteralPath (Join-Path $sourcePath $required))){throw "Missing geography resource: $required"}}
$stream=[IO.File]::Create((Join-Path $root $Output))
try{
    $writer=[IO.BinaryWriter]::new($stream)
    $writer.Write([Text.Encoding]::ASCII.GetBytes('IVGEO002'))
    $writer.Write([uint32]$files.Count)
    foreach($file in $files){$name=[Text.Encoding]::UTF8.GetBytes($file.Name);$bytes=[IO.File]::ReadAllBytes($file.FullName);$writer.Write([uint32]$name.Length);$writer.Write($name);$writer.Write([uint32]$bytes.Length);$writer.Write($bytes)}
    $writer.Flush()
}finally{$stream.Dispose()}
# Verify a round-trip byte comparison, including each PNG and its alpha channel bytes.
$stream=[IO.File]::OpenRead((Join-Path $root $Output))
try{
    $reader=[IO.BinaryReader]::new($stream)
    if([Text.Encoding]::ASCII.GetString($reader.ReadBytes(8)) -ne 'IVGEO002'){throw 'Invalid pack signature'}
    $count=$reader.ReadUInt32();if($count -ne $files.Count){throw 'Wrong file count'}
    for($i=0;$i -lt $count;$i++){$name=[Text.Encoding]::UTF8.GetString($reader.ReadBytes($reader.ReadUInt32()));$bytes=$reader.ReadBytes($reader.ReadUInt32());$original=[IO.File]::ReadAllBytes((Join-Path $sourcePath $name));$hash=[Security.Cryptography.SHA256]::Create();try{if([Convert]::ToBase64String($hash.ComputeHash($bytes)) -ne [Convert]::ToBase64String($hash.ComputeHash($original))){throw "Pack changed resource: $name"}}finally{$hash.Dispose()}}
    if($stream.Position -ne $stream.Length){throw 'Unexpected trailing bytes'}
}finally{$stream.Dispose()}
Write-Output "Geography pack verified: $count files"
