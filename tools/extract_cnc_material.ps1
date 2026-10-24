param([switch]$Refresh)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot
$sourceRoot = Join-Path $projectRoot 'inspection/cnc-modding-support-source'
$targetRoot = Join-Path $projectRoot 'references/cnc-modding-support'
$revision = '3aad8e412961ab543192ae70f8503dc8e178e095'
$repository = 'https://github.com/glitchaka/CnC_Modding_Support_oups.git'
function Invoke-GitChecked {
    param([string[]]$GitArgs)
    $result = & git @GitArgs
    if ($LASTEXITCODE -ne 0) { throw "Git failed: $GitArgs" }
    return $result
}
if (!(Test-Path -LiteralPath $sourceRoot)) {
    New-Item -ItemType Directory -Path (Split-Path $sourceRoot) -Force | Out-Null
    Invoke-GitChecked -GitArgs @('clone', '--config', 'core.autocrlf=false', '--filter=blob:none', '--no-checkout', '--depth=1', $repository, $sourceRoot)
}
if ($Refresh) { Invoke-GitChecked -GitArgs @('-C', $sourceRoot, 'fetch', '--depth=1', 'origin', $revision) }
Invoke-GitChecked -GitArgs @('-C', $sourceRoot, 'cat-file', '-e', "$revision^{commit}")
$entries = @(Invoke-GitChecked -GitArgs @('-C', $sourceRoot, 'ls-tree', '-r', '--full-tree', $revision))
$selected = @()
foreach ($entry in $entries) {
    if ($entry -notmatch '^100644 blob ([0-9a-f]+)\t(.+)$') { continue }
    $blob = $Matches[1]
    $relative = $Matches[2]
    if ($relative -match '^(README|LICENSE)\.md$' -or
        $relative -match '^[^/]+/(Schemas|Shaders|Libraries)/' -or
        $relative -match '^[^/]+/Xml/(BaseObjects|Civilian|Neutral|GlobalData|Includes|System|Templates|Terrain|SkirmishAI|FXParticleSystems|PathfinderTest|UI|UIInGame)/') {
        $selected += [pscustomobject]@{ path = $relative; git_blob = $blob }
    }
}
if ($selected.Count -lt 1000) { throw 'Unexpectedly small source inventory' }
# Select precise XML subdirectories instead of the entire Xml tree containing large maps.
$directories = @($selected | Where-Object { $_.path.Contains('/') } | ForEach-Object {
    $parts = $_.path -split '/'
    if ($parts[1] -eq 'Xml') { $parts[0..2] -join '/' } else { $parts[0..1] -join '/' }
} | Sort-Object -Unique)
Invoke-GitChecked -GitArgs @('-C', $sourceRoot, 'sparse-checkout', 'init', '--cone')
$directories | & git -C $sourceRoot sparse-checkout set --stdin
if ($LASTEXITCODE -ne 0) { throw 'Sparse checkout failed' }
Invoke-GitChecked -GitArgs @('-C', $sourceRoot, 'checkout', '--detach', $revision)
$sha1 = [System.Security.Cryptography.SHA1]::Create()
try {
    foreach ($item in $selected) {
        $sourceFile = Join-Path $sourceRoot $item.path
        $bytes = [System.IO.File]::ReadAllBytes($sourceFile)
        $header = [System.Text.Encoding]::ASCII.GetBytes("blob $($bytes.Length)`0")
        $blobBytes = New-Object byte[] ($header.Length + $bytes.Length)
        [Array]::Copy($header, 0, $blobBytes, 0, $header.Length)
        [Array]::Copy($bytes, 0, $blobBytes, $header.Length, $bytes.Length)
        $actual = [BitConverter]::ToString($sha1.ComputeHash($blobBytes)).Replace('-', '').ToLowerInvariant()
        # Reject altered bytes, including checkout line-ending conversions.
        if ($actual -ne $item.git_blob) { throw "Source bytes differ from Git blob: $($item.path). Configure core.autocrlf=false for the source checkout." }
        $targetFile = Join-Path $targetRoot $item.path
        [System.IO.Directory]::CreateDirectory((Split-Path $targetFile)) | Out-Null
        [System.IO.File]::WriteAllBytes($targetFile, $bytes)
        $item | Add-Member -NotePropertyName bytes -NotePropertyValue $bytes.Length
    }
} finally { $sha1.Dispose() }
[ordered]@{ repository = $repository; revision = $revision; files = $selected } | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $targetRoot 'MANIFEST.json') -Encoding UTF8
Write-Output "Extracted and verified $($selected.Count) files."





