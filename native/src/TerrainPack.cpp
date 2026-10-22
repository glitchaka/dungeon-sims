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
void TerrainPack::load(const std::string& path) {
    std::ifstream file(path,std::ios::binary);
    char magic[8]{};std::uint32_t metadata{},size{};
    file.read(magic,8);file.read(reinterpret_cast<char*>(&metadata),4);file.read(reinterpret_cast<char*>(&size),4);
    if(!file||std::memcmp(magic,"IVTPACK4",8)||metadata>65536||size>32*1024*1024||size<8)throw std::runtime_error("Paquete cenital invalido");
    std::string jsonText(metadata,0);file.read(jsonText.data(),metadata);auto info=nlohmann::json::parse(jsonText);
    if(info.at("camera")!="nadir_90_degrees"||info.at("columns")!=3||info.at("rows")!=3)throw std::runtime_error("Atlas cenital incompatible");
    std::vector<unsigned char> png(size);file.read(reinterpret_cast<char*>(png.data()),size);if(!file)throw std::runtime_error("Paquete cenital incompleto");
    SDL_Surface* source=IMG_Load_IO(SDL_IOFromConstMem(png.data(),png.size()),true);if(!source)throw std::runtime_error("No se pudo leer el atlas cenital");
    SDL_Surface* rgba=SDL_ConvertSurface(source,SDL_PIXELFORMAT_RGBA32);SDL_DestroySurface(source);if(!rgba)throw std::runtime_error("No se pudo convertir el atlas");
    atlasWidth_=rgba->w;atlasHeight_=rgba->h;
    if(atlasWidth_<96||atlasHeight_<96||atlasWidth_>8192||atlasHeight_>8192){SDL_DestroySurface(rgba);throw std::runtime_error("Dimensiones de atlas invalidas");}
    pixels_.resize(atlasWidth_*atlasHeight_*4);
    for(int y=0;y<atlasHeight_;++y)std::memcpy(pixels_.data()+y*atlasWidth_*4,static_cast<unsigned char*>(rgba->pixels)+y*rgba->pitch,atlasWidth_*4);
    SDL_DestroySurface(rgba);
}
std::vector<unsigned char> TerrainPack::render(const World& w) const {
    if(pixels_.empty())throw std::runtime_error("Falta el atlas cenital");
    using Color=std::array<float,3>;
    auto smooth=[](float t){t=std::clamp(t,0.0f,1.0f);return t*t*(3-2*t);};
    auto mix=[](Color a,Color b,float t){t=std::clamp(t,0.0f,1.0f);for(int c=0;c<3;++c)a[c]+=(b[c]-a[c])*t;return a;};
    int tileW=atlasWidth_/3,tileH=atlasHeight_/3;
    auto sample=[&](int tile,float u,float v){int sx=tile%3*tileW+std::clamp(static_cast<int>(u*tileW),0,tileW-1),sy=tile/3*tileH+std::clamp(static_cast<int>(v*tileH),0,tileH-1);Color c;for(int i=0;i<3;++i)c[i]=pixels_[(sy*atlasWidth_+sx)*4+i];return c;};
    std::array<int,mapWidth*mapHeight> fromSea,fromLand;fromSea.fill(999);fromLand.fill(999);
    auto distances=[&](auto& values,bool sea){std::queue<int> q;for(int k=0;k<mapWidth*mapHeight;++k)if((w.terrain[k]==Terrain::Sea)==sea){values[k]=0;q.push(k);}while(!q.empty()){int k=q.front();q.pop();for(int step:{1,-1,mapWidth,-mapWidth}){int n=k+step;if(n<0||n>=mapWidth*mapHeight||std::abs(n%mapWidth-k%mapWidth)+std::abs(n/mapWidth-k/mapWidth)!=1||values[n]<=values[k]+1)continue;values[n]=values[k]+1;q.push(n);}}};
    distances(fromSea,true);distances(fromLand,false);
    std::array<float,mapWidth*mapHeight> shore;constexpr int weights[]{1,4,6,4,1};
    for(int y=0;y<mapHeight;++y)for(int x=0;x<mapWidth;++x){float d=0;for(int dy=-2;dy<=2;++dy)for(int dx=-2;dx<=2;++dx){int k=cellId(std::clamp(x+dx,0,mapWidth-1),std::clamp(y+dy,0,mapHeight-1));d+=(w.terrain[k]==Terrain::Sea?.5f-fromLand[k]:fromSea[k]-.5f)*weights[dx+2]*weights[dy+2];}shore[cellId(x,y)]=d/256;}
    auto field=[&](int x,int y){return shore[cellId(std::clamp(x,0,mapWidth-1),std::clamp(y,0,mapHeight-1))];};
    std::vector<unsigned char> image(width*height*4);
    for(int py=0;py<height;++py)for(int px=0;px<width;++px){
        float x=px/static_cast<float>(scale),y=py/static_cast<float>(scale),cx=x-.5f,cy=y-.5f;int ix=static_cast<int>(std::floor(cx)),iy=static_cast<int>(std::floor(cy));float fx=smooth(cx-ix),fy=smooth(cy-iy);
        float d=(field(ix,iy)*(1-fx)+field(ix+1,iy)*fx)*(1-fy)+(field(ix,iy+1)*(1-fx)+field(ix+1,iy+1)*fx)*fy;
        float wave=std::sin(px*.07f+std::sin(py*.03f))*1.5f;
        Color c=mix({16+wave,78+wave,132+wave},{43,161,174},smooth((d+4)/4));
        if(d>-.3f){
            // Offset overlapping meadow modules avoid a fixed island background or a visible tile grid.
            auto fract=[](float t){return t-std::floor(t);};
            float u=fract(x/24),v=fract(y/24),blend=smooth((std::sin(x*.12f+y*.07f)+1)*.5f)*.35f;
            Color land=mix(sample(0,u,v),sample(1,fract((x+13)/31),fract((y+7)/29)),blend);
            for(std::size_t i=0;i<w.landforms.size();++i){const auto& f=w.landforms[i];float a=(x-(f.x-f.radiusX))/(2*f.radiusX),b=(y-(f.y-f.radiusY))/(2*f.radiusY);if(a<=0||a>=1||b<=0||b>=1)continue;
                int tile=f.kind=="mountain"?6+static_cast<int>(i%2):f.kind=="hill"||f.kind=="mount"?5:2;
                float opacity=smooth(std::min({a,1-a,b,1-b})/.16f);land=mix(land,sample(tile,a,b),opacity);
            }
            // Trees are independent mutable world props, never baked into the ground atlas layer.
            float river=0;
            if(w.riverCourse.size()>1){for(std::size_t i=1;i<w.riverCourse.size();++i){const auto& a=w.riverCourse[i-1];const auto& b=w.riverCourse[i];if(y<a[1]||y>b[1])continue;float t=(y-a[1])/std::max(.001f,b[1]-a[1]);float rx=a[0]+(b[0]-a[0])*t;float slope=(b[0]-a[0])/std::max(.001f,b[1]-a[1]);float distance=std::abs(x-rx)/std::sqrt(1+slope*slope);river=1-smooth((distance-.38f)/.30f);break;}}
            else if(w.terrainAt(static_cast<int>(x),static_cast<int>(y))==Terrain::FreshWater)river=1;
            land=mix(land,{40,137,161},river);
            Color beach={218,199,148};Color ground=mix(beach,land,smooth((d-.25f)/1.4f));c=mix(c,ground,smooth((d+.3f)/.5f));
        }
        c=mix(c,{230,243,226},(1-smooth(std::abs(d+.15f)/.12f))*.35f);
        int at=(py*width+px)*4;for(int i=0;i<3;++i)image[at+i]=static_cast<unsigned char>(std::clamp(c[i],0.0f,255.0f));image[at+3]=255;
    }
    return image;
}
}
