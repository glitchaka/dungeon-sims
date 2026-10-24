param([Parameter(Mandatory=$true)][string]$SourceRoot, [Parameter(Mandatory=$true)][string]$OutputRoot)
$ErrorActionPreference = 'Stop'
$SourceRoot = [IO.Path]::GetFullPath($SourceRoot).TrimEnd('\','/')
$inv = [Globalization.CultureInfo]::InvariantCulture
function Number($value) {
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) { throw 'Missing required XML number' }
    return [double]::Parse(([string]$value).TrimEnd('s','%'), $inv)
}
function Float($value) { return ([double]$value).ToString('0.0######', $inv) + 'f' }
function Read-Asset($path) {
    $doc = New-Object System.Xml.XmlDocument
    $doc.XmlResolver = $null
    $doc.Load((Join-Path $SourceRoot $path))
    return ,$doc
}
function Node($doc, $query) {
    $result = $doc.SelectSingleNode($query)
    if ($null -eq $result) { throw "Missing XML node: $query" }
    return $result
}
$base = 'Red Alert 3/Xml/'
$civilian = Read-Asset ($base + 'BaseObjects/BaseCivilian.xml')
$normal = Node $civilian "//*[local-name()='LocomotorSet' and @Condition='NORMAL']"
$panic = Node $civilian "//*[local-name()='LocomotorSet' and @Condition='PANIC']"
$ai = Node $civilian "//*[local-name()='AnimalAIUpdate']"
$body = Node $civilian "//*[local-name()='ActiveBody']"
$shadow = Node $civilian "//*[local-name()='ShadowInfo']"
$locoDoc = Read-Asset ($base + 'GlobalData/Locomotor.xml')
$loco = Node $locoDoc "//*[local-name()='LocomotorTemplate' and @id='HumanLocomotor']"
if ($loco.Surfaces -ne 'GROUND') { throw 'Unsupported civilian surface set' }
$water = Read-Asset ($base + 'GlobalData/WaterTemplates.xml')
$palette = Read-Asset ($base + 'GlobalData/MultiplayerColors.xml')
$weather = Read-Asset ($base + 'GlobalData/Weather.xml')
$rain = Node $weather "//*[local-name()='Rain']"
$treeDoc = Read-Asset ($base + 'BaseObjects/BaseTree.xml')
$treeShape = Node $treeDoc "//*[local-name()='Geometry']/*[local-name()='Shape']"
$terrain = Read-Asset ($base + 'Terrain/Terrain.xml')
$lines = [Collections.Generic.List[string]]::new()
$lines.Add('// Generated from unmodified EA XML. GPL-3.0 with additional terms: see cnc-license/LICENSE.md.')
$lines.Add('#pragma once')
$lines.Add('#include <array>')
$lines.Add('namespace isla::cnc {')
$lines.Add('inline constexpr float unitsPerTile = 24.0f; // Explicit SAGE-to-island unit conversion.')
$values = [ordered]@{
    walkSpeed = (Number $normal.Speed) / 24; panicSpeed = (Number $panic.Speed) / 24
    accelerationSeconds = (Number $loco.AccelerationSeconds)
    wanderInterval = (Number $ai.WanderUpdateTimer); wanderChance = (Number $ai.WanderPercentage) / 100
    wanderMin = (Number $ai.MinDistance) / 24; wanderMax = (Number $ai.MaxDistance) / 24
    wanderRadius = (Number $ai.Radius) / 24; fleeRadius = (Number $ai.FleeRadius) / 24
    fleeInterval = (Number $ai.FleeUpdateTimer); maxHealth = (Number $body.MaxHealth)
    shadowWidth = (Number $shadow.SizeX) / 24; shadowHeight = (Number $shadow.SizeY) / 24
    treeHalfWidth = (Number $treeShape.MajorRadius) / 24; treeHalfDepth = (Number $treeShape.MinorRadius) / 24
    treeHeight = (Number $treeShape.Height) / 24
    rainMinSpeed = (Number $rain.MinSpeed) / 24; rainMaxSpeed = (Number $rain.MaxSpeed) / 24
    rainMinAlpha = (Number $rain.MinAlpha); rainMaxAlpha = (Number $rain.MaxAlpha)
    rainDensity = (Number $rain.NumRaindropsPerBox)
}
foreach ($key in $values.Keys) { $lines.Add("inline constexpr float $key = $(Float $values[$key]);") }
$lines.Add('struct Color { unsigned char r, g, b, a; };')
$colors = @($palette.SelectNodes("//*[local-name()='MultiplayerColor']/*[local-name()='RGBColor']"))
if ($colors.Count -lt 6) { throw 'Insufficient resident colors' }
$lines.Add("inline constexpr std::array<Color, $($colors.Count)> residentColors{{")
foreach ($c in $colors) { $lines.Add("Color{$($c.R), $($c.G), $($c.B), $($c.A)},") }
$lines.Add('}};')
$lines.Add('inline constexpr std::array<Color, 4> waterLight{{')
foreach ($time in @('MORNING','AFTERNOON','EVENING','NIGHT')) {
    $c = Node $water "//*[local-name()='WaterSetting' and @TimeOfDay='$time']/*[local-name()='DiffuseColor']"
    $lines.Add("Color{$($c.R), $($c.G), $($c.B), $($c.A)},")
}
$lines.Add('}};')
$assets = @($terrain.SelectNodes("//*[local-name()='TerrainAsset']"))
$classes = @($assets | ForEach-Object { $_.Class } | Where-Object { $_ } | Sort-Object -Unique)
$lines.Add("inline constexpr int terrainAssetCount = $($assets.Count);")
$lines.Add("inline constexpr std::array<const char*, $($classes.Count)> terrainClasses{{")
foreach ($c in $classes) { $lines.Add('"' + $c + '",') }
$lines.Add('}};')
$props = @()
$unsupported = [Collections.Generic.List[object]]::new()
# Import concrete civilian buildings with geometry; model files are absent, so render 2D footprints.
$files = Get-ChildItem -LiteralPath (Join-Path $SourceRoot ($base + 'Civilian')) -Recurse -File -Filter '*.xml' | Sort-Object FullName
foreach ($file in $files) {
    if ($file.FullName -notmatch '[\\/]Buildings[\\/]') { continue }
    $doc = New-Object System.Xml.XmlDocument
    $doc.XmlResolver = $null
    try { $doc.Load($file.FullName) } catch {
        $unsupported.Add([pscustomobject]@{ source = $file.FullName.Substring($SourceRoot.Length).TrimStart('\','/'); reason = $_.Exception.Message })
        continue
    }
    $obj = $doc.SelectSingleNode("//*[local-name()='GameObject']")
    $shape = $doc.SelectSingleNode("//*[local-name()='Geometry']/*[local-name()='Shape']")
    if (!$obj -or !$shape -or !$shape.MajorRadius -or !$shape.Height) { continue }
    if ($shape.MajorRadius -notmatch '^\d+(\.\d+)?$' -or ($shape.MinorRadius -and $shape.MinorRadius -notmatch '^\d+(\.\d+)?$') -or $shape.Height -notmatch '^\d+(\.\d+)?$') { continue }
    $width = Number $shape.MajorRadius
    $depth = if ($shape.MinorRadius) { Number $shape.MinorRadius } else { $width }
    if ($width -le 0 -or $depth -le 0) { continue }
    $props += [pscustomobject]@{ id = [string]$obj.id; halfWidth = $width / 24; halfDepth = $depth / 24; height = (Number $shape.Height) / 24; source = $file.FullName.Substring($SourceRoot.Length).TrimStart('\','/').Replace('\','/') }

}
if ($props.Count -lt 6) { throw 'Insufficient building geometries' }
$lines.Add('struct Building { const char* id; float halfWidth, halfDepth, height; };')
$lines.Add("inline constexpr std::array<Building, $($props.Count)> buildings{{")
foreach ($prop in $props) { $lines.Add('Building{"' + $prop.id + '", ' + (Float $prop.halfWidth) + ', ' + (Float $prop.halfDepth) + ', ' + (Float $prop.height) + '},') }
$lines.Add('}};')
$trees = @()
foreach ($file in $files) {
    if ($file.FullName -notmatch '[\\/](Trees|Shrubbery|Vegetation|Foliage)[\\/]') { continue }
    $doc = New-Object System.Xml.XmlDocument
    $doc.XmlResolver = $null
    try { $doc.Load($file.FullName) } catch {
        $unsupported.Add([pscustomobject]@{ source = $file.FullName.Substring($SourceRoot.Length).TrimStart('\','/'); reason = $_.Exception.Message })
        continue
    }
    $obj = $doc.SelectSingleNode("//*[local-name()='GameObject']")
    if (!$obj) { continue }
    $shape = $doc.SelectSingleNode("//*[local-name()='Geometry']/*[local-name()='Shape']")
    if (!$shape) { $shape = $treeShape }
    if ($shape.MajorRadius -notmatch '^\d+(\.\d+)?$' -or $shape.Height -notmatch '^\d+(\.\d+)?$') { continue }
    $width = Number $shape.MajorRadius
    $depth = if ($shape.MinorRadius -match '^\d+(\.\d+)?$') { Number $shape.MinorRadius } else { $width }
    $trees += [pscustomobject]@{ id = [string]$obj.id; halfWidth = $width / 24; halfDepth = $depth / 24; height = (Number $shape.Height) / 24; source = $file.FullName.Substring($SourceRoot.Length).TrimStart('\','/').Replace('\','/') }
}
if (!$trees.Count) { throw 'No civilian trees imported' }
$lines.Add("inline constexpr std::array<Building, $($trees.Count)> trees{{")
foreach ($tree in $trees) { $lines.Add('Building{"' + $tree.id + '", ' + (Float $tree.halfWidth) + ', ' + (Float $tree.halfDepth) + ', ' + (Float $tree.height) + '},') }
$lines.Add('}};')
$lines.Add('} // namespace isla::cnc')
[IO.Directory]::CreateDirectory($OutputRoot) | Out-Null
$lines | Set-Content -LiteralPath (Join-Path $OutputRoot 'CncData.h') -Encoding UTF8
[ordered]@{ revision = '3aad8e412961ab543192ae70f8503dc8e178e095'; source = 'glitchaka/CnC_Modding_Support_oups'; unitsPerTile = 24; parameters = $values; buildings = $props; trees = $trees; unsupported = @($unsupported); terrainAssets = $assets.Count; terrainClasses = $classes; adaptation = 'Original numeric XML values; SAGE distance units divided by 24. Buildings use 2D geometry proxies. No original ART resources or SAGE shaders are executed.' } | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $OutputRoot 'cnc-import.json') -Encoding UTF8
Write-Output "Imported civilian locomotion, health, shadows, tree geometry, water, palette, weather, $($props.Count) buildings, $($trees.Count) trees and $($assets.Count) terrain definitions."


