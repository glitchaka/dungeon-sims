#include "TerrainPack.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <nlohmann/json.hpp>
#include <queue>
#include <stdexcept>
namespace isla {
void TerrainPack::load(const std::string& path){
    std::ifstream file(path,std::ios::binary);char magic[8]{};std::uint32_t metadata{},size{};file.read(magic,8);file.read(reinterpret_cast<char*>(&metadata),4);file.read(reinterpret_cast<char*>(&size),4);
    if(std::memcmp(magic,"IVTPACK2",8)||metadata>65536||size>32*1024*1024||size<8)throw std::runtime_error("Paquete de terrenos invalido");std::string jsonText(metadata,0);file.read(jsonText.data(),metadata);auto info=nlohmann::json::parse(jsonText);info.at("reliefRects").get_to(reliefRects);std::vector<unsigned char> png(size);file.read(reinterpret_cast<char*>(png.data()),size);if(!file)throw std::runtime_error("Paquete de terrenos incompleto");
    SDL_Surface* source=IMG_Load_IO(SDL_IOFromConstMem(png.data(),png.size()),true);if(!source)throw std::runtime_error("No se pudo leer el atlas de terreno");SDL_Surface* rgba=SDL_ConvertSurface(source,SDL_PIXELFORMAT_RGBA32);SDL_DestroySurface(source);if(!rgba)throw std::runtime_error("No se pudo convertir el atlas");atlasWidth_=rgba->w;atlasHeight_=rgba->h;pixels_.resize(atlasWidth_*atlasHeight_*4);for(int y=0;y<atlasHeight_;++y)std::memcpy(pixels_.data()+y*atlasWidth_*4,static_cast<unsigned char*>(rgba->pixels)+y*rgba->pitch,atlasWidth_*4);SDL_DestroySurface(rgba);
    file.read(reinterpret_cast<char*>(&size),4);if(size>32*1024*1024||size<8)throw std::runtime_error("Relieve de terreno invalido");png.resize(size);file.read(reinterpret_cast<char*>(png.data()),size);if(!file)throw std::runtime_error("Relieve incompleto");
    source=IMG_Load_IO(SDL_IOFromConstMem(png.data(),png.size()),true);if(!source)throw std::runtime_error("No se pudo leer el relieve");rgba=SDL_ConvertSurface(source,SDL_PIXELFORMAT_RGBA32);SDL_DestroySurface(source);if(!rgba)throw std::runtime_error("Relieve incompatible");reliefWidth=rgba->w;reliefHeight=rgba->h;relief.resize(reliefWidth*reliefHeight*4);for(int y=0;y<reliefHeight;++y)std::memcpy(relief.data()+y*reliefWidth*4,static_cast<unsigned char*>(rgba->pixels)+y*rgba->pitch,reliefWidth*4);SDL_DestroySurface(rgba);
}
std::vector<unsigned char> TerrainPack::render(const World& w) const{
    if(pixels_.empty())throw std::runtime_error("Falta el atlas de terrenos");
    std::array<int,mapWidth*mapHeight> fromSea,fromLand;fromSea.fill(999);fromLand.fill(999);
    auto distances=[&](std::array<int,mapWidth*mapHeight>& values,bool sea){std::queue<int> q;for(int k=0;k<mapWidth*mapHeight;++k)if((w.terrain[k]==Terrain::Sea)==sea){values[k]=0;q.push(k);}while(!q.empty()){int k=q.front();q.pop();for(int d:{1,-1,mapWidth,-mapWidth}){int n=k+d;if(n<0||n>=mapWidth*mapHeight||std::abs(n%mapWidth-k%mapWidth)+std::abs(n/mapWidth-k/mapWidth)!=1||values[n]<=values[k]+1)continue;values[n]=values[k]+1;q.push(n);}}};distances(fromSea,true);distances(fromLand,false);
    std::vector<unsigned char> image(width*height*4);int tileW=atlasWidth_/4,tileH=atlasHeight_/2;
    auto sample=[&](int material,int px,int py){int tw=tileW-16,th=tileH-16;auto mirror=[](int p,int size){int t=p%(size*2);return t<size?t:size*2-1-t;};int sx=(material%4)*tileW+8+mirror(px*3/2,tw),sy=(material/4)*tileH+8+mirror(py*3/2,th);std::array<float,3> c;for(int i=0;i<3;++i)c[i]=pixels_[(sy*atlasWidth_+sx)*4+i];const std::array<std::array<float,3>,8> palette{{{14,84,139},{33,165,182},{210,188,133},{126,141,68},{119,117,101},{213,222,224},{46,146,174},{111,82,43}}};float detail=material==0?.19f:material==3?.30f:.55f;for(int i=0;i<3;++i)c[i]=palette[material][i]+(c[i]-palette[material][i])*detail;return c;};
    auto mix=[](std::array<float,3> a,const std::array<float,3>& b,float t){t=std::clamp(t,0.0f,1.0f);for(int i=0;i<3;++i)a[i]+=(b[i]-a[i])*t;return a;};
    std::array<float,mapWidth*mapHeight> shoreline;
    constexpr int weights[]{1,4,6,4,1};
    for(int y=0;y<mapHeight;++y)for(int x=0;x<mapWidth;++x){float d=0;for(int dy=-2;dy<=2;++dy)for(int dx=-2;dx<=2;++dx){int k=cellId(std::clamp(x+dx,0,mapWidth-1),std::clamp(y+dy,0,mapHeight-1));d+=(w.terrain[k]==Terrain::Sea?.5f-fromLand[k]:fromSea[k]-.5f)*weights[dx+2]*weights[dy+2];}shoreline[cellId(x,y)]=d/256;}
    auto field=[&](int x,int y){return shoreline[cellId(std::clamp(x,0,mapWidth-1),std::clamp(y,0,mapHeight-1))];};
    std::vector<float> riverX(height,-999),riverH(height,0);
    if(w.riverCourse.size()>1)for(int py=0;py<height;++py){float y=py/static_cast<float>(scale);if(y<w.riverCourse.front()[1]||y>w.riverCourse.back()[1])continue;int i=std::clamp(static_cast<int>(std::floor(y-w.riverCourse.front()[1])),0,static_cast<int>(w.riverCourse.size())-2);auto a=w.riverCourse[i],b=w.riverCourse[i+1];float t=std::clamp(y-a[1],0.0f,1.0f);riverX[py]=a[0]+(b[0]-a[0])*t;riverH[py]=a[2]+(b[2]-a[2])*t;}
    for(int py=0;py<height;++py)for(int px=0;px<width;++px){auto water=sample(0,px,py);for(int i=0;i<3;++i)image[(py*width+px)*4+i]=static_cast<unsigned char>(water[i]);image[(py*width+px)*4+3]=255;}
    for(int py=0;py<height;++py)for(int px=0;px<width;++px){float x=px/static_cast<float>(scale)-.5f,y=py/static_cast<float>(scale)-.5f;int ix=static_cast<int>(std::floor(x)),iy=static_cast<int>(std::floor(y));float fx=x-ix,fy=y-iy;fx=fx*fx*(3-2*fx);fy=fy*fy*(3-2*fy);float top=field(ix,iy)*(1-fx)+field(ix+1,iy)*fx,bottom=field(ix,iy+1)*(1-fx)+field(ix+1,iy+1)*fx,d=top*(1-fy)+bottom*fy;
        auto c=mix(sample(0,px,py),sample(1,px,py),std::clamp((d+5)/4,0.0f,1.0f));
        c=mix(c,sample(2,px,py),std::clamp((d+.15f)*2.5f,0.0f,1.0f));c=mix(c,sample(3,px,py),std::clamp((d-.8f)/1.5f,0.0f,1.0f));
        int gx=std::clamp(px/scale,0,mapWidth-1),gy=std::clamp(py/scale,0,mapHeight-1);float mountain=0,river=0;for(int dy=0;dy<=1;++dy)for(int dx=0;dx<=1;++dx){auto t=w.terrainAt(ix+dx,iy+dy);float weight=(dx?fx:1-fx)*(dy?fy:1-fy);if(t==Terrain::Mountain)mountain+=weight;if(t==Terrain::FreshWater||t==Terrain::Bridge)river+=weight;}
        if(!w.riverCourse.empty())river=riverX[py]>-900?std::clamp((.78f-std::abs(px/static_cast<float>(scale)-riverX[py]))/.23f,0.0f,1.0f):0;
        if(mountain>0)c=mix(c,mix(sample(3,px,py),sample(4,px,py),.65f),mountain*.75f);if(river>0)c=mix(c,sample(6,px,py),std::min(1.0f,river*1.6f));
        float foam=std::max(0.0f,1-std::abs(d+.15f)*7)*.28f;c=mix(c,{235,250,236},foam);
        float wx=px/static_cast<float>(scale),wy=py/static_cast<float>(scale);
        float h=w.heightAt(wx,wy),south=w.heightAt(wx,wy+1.0f/scale);if(river>.8f&&!w.riverCourse.empty()){h=riverH[py];south=riverH[std::min(py+1,height-1)];}
        float light=std::clamp(1.0f+(w.heightAt(wx-.5f,wy)-w.heightAt(wx+.5f,wy))*.23f+(w.heightAt(wx,wy-.5f)-w.heightAt(wx,wy+.5f))*.18f,.52f,1.28f);
        int screen=py-static_cast<int>(h*scale*.7f);int front=py+1-static_cast<int>(south*scale*.7f);int end=std::min(height-1,std::max(screen,front));
        for(int yy=std::max(0,screen);yy<=end;++yy){auto surface=c;
            if((h-south)*scale>.45f||(yy>screen&&h>south+.01f)){surface=sample(river>.2f?6:4,px,py+yy-screen);for(float& channel:surface)channel*=.68f;}else for(float& channel:surface)channel*=light;
            for(int i=0;i<3;++i)image[(yy*width+px)*4+i]=static_cast<unsigned char>(std::clamp(surface[i],0.0f,255.0f));image[(yy*width+px)*4+3]=255;
        }
    }
    return image;
}
}









