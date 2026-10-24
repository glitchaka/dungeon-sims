param([string]$Layout='native/previews/assembled-island.json',[string]$Output='native/previews/assembled-island.png')
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
$root=Split-Path $PSScriptRoot
$assets=Join-Path $root 'native/assets/archive/terrain-blocks-v3'
$catalog=Get-Content (Join-Path $assets 'manifest.json') -Raw | ConvertFrom-Json
$map=Get-Content (Join-Path $root $Layout) -Raw | ConvertFrom-Json
$blocks=@{};foreach($b in $catalog.blocks){$blocks[$b.id]=$b}
$images=@{};foreach($sheet in $catalog.sheets){$images[$sheet]=[Drawing.Image]::FromFile((Join-Path $assets $sheet))}
$canvas=[Drawing.Bitmap]::new(1800,1400)
$g=[Drawing.Graphics]::FromImage($canvas)
$g.Clear([Drawing.Color]::FromArgb(15,92,132))
$g.InterpolationMode=[Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$g.CompositingQuality=[Drawing.Drawing2D.CompositingQuality]::HighQuality
try {
 foreach($p in ($map.placements | Sort-Object @{Expression={$_.x+$_.y}},y)){
  $b=$blocks[$p.block];if(-not $b){throw "Unknown block $($p.block)"}
  $r=$b.sourceRect
  $fw=1; $fh=1;foreach($c in $b.footprint){$fw=[Math]::Max($fw,1+[int]$c[0]);$fh=[Math]::Max($fh,1+[int]$c[1])}
  $w=($fw+$fh)*90.0*1.20
  if($p.scale){$w*=[double]$p.scale}
  $h=$w*[double]$r[3]/[double]$r[2]
  $cx=900+([double]$p.x-[double]$p.y)*90
  $bottom=170+([double]$p.x+[double]$p.y)*52+85
  $dst=[Drawing.RectangleF]::new([single]($cx-$w/2),[single]($bottom-$h),[single]$w,[single]$h)
  $src=[Drawing.RectangleF]::new([single]$r[0],[single]$r[1],[single]$r[2],[single]$r[3]); $g.DrawImage($images[$b.sheet],$dst,$src,[Drawing.GraphicsUnit]::Pixel)
 }
 $canvas.Save((Join-Path $root $Output),[Drawing.Imaging.ImageFormat]::Png)
} finally {$g.Dispose();$canvas.Dispose();foreach($i in $images.Values){$i.Dispose()}}
Write-Output "Rendered $($map.placements.Count) existing atlas pieces to $Output"



