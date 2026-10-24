$ErrorActionPreference='Stop'
$root=Join-Path (Split-Path $PSScriptRoot -Parent) 'legacy/godot'
$mapPath=Join-Path $root 'Res/The Fan-tasy Tileset (Exteriores)/Tiled/Tilemaps/Beginning Fields.tmx'
[xml]$map=Get-Content -LiteralPath $mapPath
function ResourcePath($base,$relative){
    $full=[IO.Path]::GetFullPath((Join-Path $base $relative))
    return 'res://'+$full.Substring($root.Length+1).Replace('\','/')
}
$tiles=@{}
foreach($set in $map.map.tileset){
    $tsxPath=[IO.Path]::GetFullPath((Join-Path (Split-Path $mapPath) $set.source))
    [xml]$tsx=Get-Content -LiteralPath $tsxPath
    $first=[int]$set.firstgid;$ts=$tsx.tileset
    if($ts.image){
        $path=ResourcePath (Split-Path $tsxPath) $ts.image.source
        $cols=[int]$ts.columns;$tw=[int]$ts.tilewidth;$th=[int]$ts.tileheight
        for($i=0;$i -lt [int]$ts.tilecount;$i++){
            $tiles[[string]($first+$i)]=@{path=$path;rect=@((($i%$cols)*$tw),(([math]::Floor($i/$cols))*$th),$tw,$th);polygons=@()}
        }
    }
    foreach($tile in $ts.tile){
        if($tile.image){
            $path=ResourcePath (Split-Path $tsxPath) $tile.image.source
            $polygons=@()
            foreach($obj in $tile.objectgroup.object){
                if($obj.polygon){
                    $pts=@();foreach($point in $obj.polygon.points.Split(' ')){$xy=$point.Split(',');$pts+=,@(([double]$obj.x+[double]$xy[0]),([double]$obj.y+[double]$xy[1]))};$polygons+=,@($pts)
                }elseif($obj.width -and $obj.height){
                    $x=[double]$obj.x;$y=[double]$obj.y;$w=[double]$obj.width;$h=[double]$obj.height
                    $polygons+=,@(@($x,$y),@(($x+$w),$y),@(($x+$w),($y+$h)),@($x,($y+$h)))
                }
            }
            $tiles[[string]($first+[int]$tile.id)]=@{path=$path;rect=@(0,0,[int]$tile.image.width,[int]$tile.image.height);polygons=$polygons}
        }
    }
}
$layers=@()
foreach($layer in $map.SelectNodes('//map/layer | //map/group/layer')){
    $values=@();foreach($n in $layer.data.InnerText.Trim().Split(',')){$values+= [long]$n.Trim()}
    $alpha=1.0; if($layer.ParentNode.GetAttribute('opacity')){$alpha=[double]$layer.ParentNode.GetAttribute('opacity')}; $layers+=@{name=$layer.GetAttribute('name');cells=$values;alpha=$alpha;visible=($layer.GetAttribute('visible') -ne '0')}
}
$objects=@()
foreach($obj in $map.map.objectgroup.object){
    if($obj.gid){$objects+=@{id=[int]$obj.id;gid=[long]$obj.gid;pos=@([double]$obj.x,([double]$obj.y-[double]$obj.height));size=@([double]$obj.width,[double]$obj.height)}}
}
$shadows=@()
foreach($obj in $map.SelectNodes('//map/group/objectgroup/object')){
    if($obj.gid){$shadows+=@{gid=[long]$obj.gid;pos=@([double]$obj.x,([double]$obj.y-[double]$obj.height));size=@([double]$obj.width,[double]$obj.height)}}
}
$data=@{width=40;height=40;tiles=$tiles;layers=$layers;objects=$objects;shadows=$shadows}
$json=$data|ConvertTo-Json -Depth 24 -Compress
[IO.File]::WriteAllText((Join-Path $root 'data/town.json'),$json,(New-Object Text.UTF8Encoding $false))
Write-Output "Converted $($layers.Count) layers, $($objects.Count) objects, $($tiles.Count) tile definitions."


