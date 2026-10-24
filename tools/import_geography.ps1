param([Parameter(Mandatory=$true)][string]$Image,[Parameter(Mandatory=$true)][string]$Output)
$ErrorActionPreference='Stop'
Remove-Item Env:LIB,Env:LIBPATH -ErrorAction SilentlyContinue
Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @"
using System;
using System.Drawing;
public static class Geography {
 public static int[] Read(string file) {
  using(var image=new Bitmap(file)) {
   int[] cells=new int[128*96];
   int[,] river={{62,13},{67,21},{66,28},{66,32},{70,40},{76,45},{79,51},{77,55},{72,58},{69,61},{70,65},{74,69},{73,74},{74,78}};
   for(int y=0;y<96;y++)for(int x=0;x<128;x++) {
    int water=0,rock=0;
    for(int sy=0;sy<5;sy++)for(int sx=0;sx<5;sx++) {
     int px=Math.Min(image.Width-1,(int)((x+(sx+.5)/5)*image.Width/128));
     int py=Math.Min(image.Height-1,(int)((y+(sy+.5)/5)*image.Height/96));
     var c=image.GetPixel(px,py);
     bool wet=c.B>c.R*1.25&&c.B>c.G*.84;
     if(wet)water++;
     else if(c.R>65&&c.G>60&&c.B>50&&c.B>c.G*.76&&Math.Abs(c.R-c.G)<45)rock++;
    }
    int type=water>=12?0:rock>=12?5:1;
    if(type==0) {
     double best=1000;
     for(int i=1;i<river.GetLength(0);i++){
      double ax=river[i-1,0],ay=river[i-1,1],dx=river[i,0]-ax,dy=river[i,1]-ay;
      double t=Math.Max(0,Math.Min(1,((x+.5-ax)*dx+(y+.5-ay)*dy)/(dx*dx+dy*dy)));
      double distance=Math.Sqrt(Math.Pow(x+.5-ax-t*dx,2)+Math.Pow(y+.5-ay-t*dy,2));best=Math.Min(best,distance);
     }
     if(best<3.2&&y<79)type=2;
    }
    cells[y*128+x]=type;
   }
   return cells;
  }
 }
}
"@
$cells=[Geography]::Read($Image)
[IO.Directory]::CreateDirectory((Split-Path $Output)) | Out-Null
$header='#pragma once'+[Environment]::NewLine+'#include <array>'+[Environment]::NewLine+'namespace isla { inline constexpr std::array<int,12288> geologyCells{{'+[Environment]::NewLine+($cells -join ',')+[Environment]::NewLine+'}}; }'+[Environment]::NewLine
[IO.File]::WriteAllText($Output,$header,[Text.UTF8Encoding]::new($false))
'Imported terrain collision mask from geology asset'


