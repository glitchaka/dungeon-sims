param([Parameter(Mandatory=$true)][string]$SourceRoot,[Parameter(Mandatory=$true)][string]$BuildRoot)
$ErrorActionPreference='Stop'
$testRoot=Join-Path $BuildRoot ('import-test-'+[Guid]::NewGuid().ToString('N'))
$inputRoot=Join-Path $testRoot 'source'
$manifest=Get-Content (Join-Path $BuildRoot 'generated/cnc-import.json') -Raw | ConvertFrom-Json
$files=@('BaseObjects/BaseCivilian.xml','BaseObjects/BaseTree.xml','GlobalData/Locomotor.xml','GlobalData/WaterTemplates.xml','GlobalData/MultiplayerColors.xml','GlobalData/Weather.xml','Terrain/Terrain.xml') | ForEach-Object { 'Red Alert 3/Xml/'+$_ }
$files+=@(@($manifest.buildings)+@($manifest.trees) | ForEach-Object { $_.source })
foreach($file in $files) {
    $target=Join-Path $inputRoot $file
    [IO.Directory]::CreateDirectory((Split-Path $target)) | Out-Null
    Copy-Item -LiteralPath (Join-Path $SourceRoot $file) -Destination $target
}
$civilianPath=Join-Path $inputRoot 'Red Alert 3/Xml/BaseObjects/BaseCivilian.xml'
[xml]$doc=Get-Content -LiteralPath $civilianPath -Raw
$normal=$doc.SelectSingleNode("//*[local-name()='LocomotorSet' and @Condition='NORMAL']")
$normal.SetAttribute('Speed','34.0')
$doc.Save($civilianPath)
& (Join-Path $PSScriptRoot 'import_cnc_data.ps1') -SourceRoot $inputRoot -OutputRoot (Join-Path $testRoot 'generated')
$changed=Get-Content (Join-Path $testRoot 'generated/cnc-import.json') -Raw | ConvertFrom-Json
if([Math]::Abs($changed.parameters.walkSpeed-34.0/24.0) -gt .00001) { throw 'XML change did not affect runtime walk speed' }
if($changed.parameters.maxHealth -ne $manifest.parameters.maxHealth) { throw 'Unrelated XML values changed' }
# Missing required source must fail instead of silently reverting to hard-coded defaults.
$failed=$false
try { & (Join-Path $PSScriptRoot 'import_cnc_data.ps1') -SourceRoot (Join-Path $testRoot 'missing') -OutputRoot (Join-Path $testRoot 'invalid') } catch { $failed=$true }
if(!$failed) { throw 'Importer accepted missing XML sources' }
'PASS: source XML mutations propagate and missing inputs fail'
