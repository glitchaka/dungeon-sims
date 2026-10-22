param([ValidateSet('rounded','elongated','crescent','archipelago')][string]$Pattern='rounded',[int]$Seed=1749,[int]$Width=24,[int]$Height=20,[string]$Output)
$ErrorActionPreference='Stop'
if($Width -lt 8 -or $Height -lt 8 -or $Width -gt 128 -or $Height -gt 128){throw 'Layout dimensions must be 8..128'}
$root=Split-Path $PSScriptRoot
$catalog=Get-Content -LiteralPath (Join-Path $root 'native/assets/terrain-blocks-v3/manifest.json') -Raw | ConvertFrom-Json
$vertices=[bool[]]::new(($Width+1)*($Height+1));$phase=($Seed%10000)*.011
for($y=0;$y -le $Height;$y++){for($x=0;$x -le $Width;$x++){
    $nx=($x-$Width/2.0)/($Width/2.0-1);$ny=($y-$Height/2.0)/($Height/2.0-1)
    $r=[Math]::Sqrt($nx*$nx+$ny*$ny);$edge=.78+.06*[Math]::Sin([Math]::Atan2($ny,$nx)*5+$phase)
    $land=switch($Pattern){
        'rounded' {$r -lt $edge}
        'elongated' {[Math]::Sqrt($nx*$nx+$ny*$ny/.36) -lt $edge}
        'crescent' {$r -lt .9 -and (($nx-.42)*($nx-.42)+($ny+.08)*($ny+.08)) -gt .39}
        'archipelago' {($nx+.43)*($nx+.43)/.12+($ny+.3)*($ny+.3)/.16 -lt 1 -or ($nx-.4)*($nx-.4)/.13+($ny-.3)*($ny-.3)/.17 -lt 1}
    }
    $vertices[$y*($Width+1)+$x]=$land -and $x -gt 0 -and $y -gt 0 -and $x -lt $Width -and $y -lt $Height
}}
function Get-CornerMask([int]$x,[int]$y){$mask=0;$indices=@(($y*($Width+1)+$x),($y*($Width+1)+$x+1),(($y+1)*($Width+1)+$x+1),(($y+1)*($Width+1)+$x));for($c=0;$c -lt 4;$c++){if($vertices[$indices[$c]]){$mask=$mask -bor (1 -shl $c)}};return $mask}
# Remove ambiguous diagonal contacts, keeping a single shared vertex field for every seam.
for($pass=0;$pass -lt 8;$pass++){$changes=0;for($y=0;$y -lt $Height;$y++){for($x=0;$x -lt $Width;$x++){$m=Get-CornerMask $x $y;if($m -eq 5 -or $m -eq 10){$vertices[$y*($Width+1)+$x]=$vertices[$y*($Width+1)+$x+1];$changes++}}};if($changes -eq 0){break}}
$masks=[int[]]::new($Width*$Height);$used=[bool[]]::new($Width*$Height);$placements=[Collections.Generic.List[object]]::new()
$coasts=@{};foreach($block in $catalog.blocks){if($block.category -eq 'coast'){$coasts[[int]$block.cornerMask]=$block}}
for($y=0;$y -lt $Height;$y++){for($x=0;$x -lt $Width;$x++){$masks[$y*$Width+$x]=Get-CornerMask $x $y}}
# Cover interiors with differently sized modules rather than repeating a single image.
$pieces=@($catalog.blocks | Where-Object {$_.category -eq 'interior'})
$random=[Random]::new($Seed)
for($y=0;$y -lt $Height;$y++){for($x=0;$x -lt $Width;$x++){
    $index=$y*$Width+$x;$mask=$masks[$index];if($used[$index] -or $mask -eq 0){continue}
    if($mask -ne 15){if(-not $coasts.ContainsKey($mask)){throw "Unsupported coast mask $mask"};$placements.Add([pscustomobject]@{block=$coasts[$mask].id;x=$x;y=$y});$used[$index]=$true;continue}
    $ordered=@($pieces | Sort-Object { $random.Next() })
    foreach($candidate in $ordered){$fits=$true;foreach($cell in $candidate.footprint){$xx=$x+[int]$cell[0];$yy=$y+[int]$cell[1];if($xx -ge $Width -or $yy -ge $Height -or $used[$yy*$Width+$xx] -or $masks[$yy*$Width+$xx] -ne 15){$fits=$false;break}}
        if(-not $fits){continue};$placements.Add([pscustomobject]@{block=$candidate.id;x=$x;y=$y});foreach($cell in $candidate.footprint){$used[($y+[int]$cell[1])*$Width+$x+[int]$cell[0]]=$true};break
    }
}}
for($i=0;$i -lt $masks.Length;$i++){if($masks[$i] -ne 0 -and -not $used[$i]){throw "Uncovered terrain cell $i"}}
for($y=0;$y -lt $Height;$y++){for($x=0;$x -lt $Width;$x++){$m=$masks[$y*$Width+$x];if($x+1 -lt $Width){$n=$masks[$y*$Width+$x+1];if((($m -shr 1) -band 1) -ne ($n -band 1) -or (($m -shr 2) -band 1) -ne (($n -shr 3) -band 1)){throw 'Horizontal shoreline connection mismatch'}};if($y+1 -lt $Height){$n=$masks[($y+1)*$Width+$x];if((($m -shr 3) -band 1) -ne ($n -band 1) -or (($m -shr 2) -band 1) -ne (($n -shr 1) -band 1)){throw 'Vertical shoreline connection mismatch'}}}}
$result=[ordered]@{format='IslaViva modular layout';pattern=$Pattern;seed=$Seed;width=$Width;height=$Height;catalog='../manifest.json';status='logical_composition_not_runtime_island';placements=$placements.ToArray()}
if(-not $Output){$Output="native/assets/terrain-blocks-v3/layouts/$Pattern.json"}
ConvertTo-Json -InputObject $result -Depth 8 | Set-Content -LiteralPath (Join-Path $root $Output) -Encoding UTF8
Write-Output "$Pattern : $($placements.Count) modules; coverage and shoreline connections verified"
