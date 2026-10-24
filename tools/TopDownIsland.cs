using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
public static class TopDownIsland {
 static double Clamp(double x) { return Math.Max(0,Math.Min(1,x)); }
 static double Smooth(double x) { x=Clamp(x); return x*x*(3-2*x); }
 static double Noise(int x,int y,int seed) { unchecked { uint h=(uint)(x*374761393+y*668265263+seed*1447); h=(h^(h>>13))*1274126177; return (h^(h>>16))/(double)uint.MaxValue; } }
 static double Land(double x,double y,int seed) {
  double nx=(x-768)/610,ny=(y-650)/490;
  double theta=Math.Atan2(ny,nx),r=Math.Sqrt(nx*nx+ny*ny);
  double edge=.88+.075*Math.Sin(theta*5+seed*.01)+.035*Math.Sin(theta*11+.8);
  edge-=.15*Math.Exp(-Math.Pow((theta-.05)/.24,2));
  edge-=.12*Math.Exp(-Math.Pow(Math.Atan2(Math.Sin(theta+2.8),Math.Cos(theta+2.8))/.28,2));
  return (edge-r)*490;
 }
 public static void Render(string atlas,string output,int seed,double[] regions) {
  const int W=1536,H=1280;
  using(var input=new Bitmap(atlas)) using(var source=new Bitmap(input.Width,input.Height,PixelFormat.Format32bppArgb)) using(var dest=new Bitmap(W,H,PixelFormat.Format32bppArgb)) {
   using(var g=Graphics.FromImage(source)){g.DrawImageUnscaled(input,0,0);}
   var rect=new Rectangle(0,0,source.Width,source.Height);
   var bits=source.LockBits(rect,ImageLockMode.ReadOnly,PixelFormat.Format32bppArgb);
   byte[] src=new byte[bits.Stride*bits.Height]; Marshal.Copy(bits.Scan0,src,0,src.Length); int stride=bits.Stride;source.UnlockBits(bits);
   int cw=source.Width/3,ch=source.Height/3;
   byte[] pixels=new byte[W*H*4];
   for(int y=0;y<H;y++)for(int x=0;x<W;x++) {
    double n=Noise(x,y,seed),dist=Land(x,y,seed);
    double[] col=new double[3];
    double shallow=Smooth((dist+48)/50),wave=Math.Sin(x*.14+Math.Sin(y*.03)*3+y*.10)*.8+(n-.5)*3;
    col[0]=132+(154-132)*shallow+wave;col[1]=75+(171-75)*shallow+wave;col[2]=19+(54-19)*shallow+wave;
    if(dist>-2) {
     double[] grass=new double[3];
     int sx=(x%cw),sy=(y%ch);
     int si=sy*stride+sx*4;
     for(int c=0;c<3;c++)grass[c]=src[si+c];
     foreach(int c in new int[]{0,1,2}) { double variation=Smooth((Math.Sin(x*.008+y*.004)+1)*.5)*.23;grass[c]=grass[c]*(1-variation)+src[sy*stride+(cw+sx)*4+c]*variation; }
     for(int ri=0;ri<regions.Length;ri+=5) {
      int tile=(int)regions[ri];double rx=regions[ri+1],ry=regions[ri+2],rw=regions[ri+3],rh=regions[ri+4];
      double u=(x-rx)/rw,v=(y-ry)/rh;if(u<0||u>=1||v<0||v>=1)continue;
      double a=Smooth(Math.Min(Math.Min(u,1-u),Math.Min(v,1-v))/.14);
      int tx=tile%3*cw+Math.Min(cw-1,(int)(u*cw)),ty=tile/3*ch+Math.Min(ch-1,(int)(v*ch));
      int at=ty*stride+tx*4;for(int c=0;c<3;c++)grass[c]=grass[c]*(1-a)+src[at+c]*a;
     }
     double sand=1-Smooth((dist-4)/19),landA=Smooth((dist+2)/4);
     double[] beach={151+(n-.5)*12,202+(n-.5)*12,222+(n-.5)*12};
     for(int c=0;c<3;c++)col[c]=col[c]*(1-landA)+(grass[c]*(1-sand)+beach[c]*sand)*landA;
     // A continuous meandering watercourse is drawn in world coordinates, never from a row of river stamps.
     if(y>460&&y<1160) {
      double t=(y-460)/700.0,riverX=800+Math.Sin(t*8)*46+Math.Sin(t*19)*13;
      double width=4+t*6,rd=Math.Abs(x-riverX);
      double bank=1-Smooth((rd-width)/5),water=1-Smooth((rd-width+2)/2);
      if(dist>0)for(int c=0;c<3;c++) { double shore=c==0?125:c==1?158:164; double stream=c==0?158:c==1?134:49;col[c]=col[c]*(1-bank)+shore*bank;col[c]=col[c]*(1-water)+stream*water; }
     }
    }
    double foam=(1-Smooth(Math.Abs(dist+1)/2))*.40;
    int p=(y*W+x)*4;for(int c=0;c<3;c++)pixels[p+c]=(byte)Math.Max(0,Math.Min(255,col[c]*(1-foam)+239*foam));pixels[p+3]=255;
   }
   var db=dest.LockBits(new Rectangle(0,0,W,H),ImageLockMode.WriteOnly,PixelFormat.Format32bppArgb);Marshal.Copy(pixels,0,db.Scan0,pixels.Length);dest.UnlockBits(db);dest.Save(output,ImageFormat.Png);
  }
 }
}

