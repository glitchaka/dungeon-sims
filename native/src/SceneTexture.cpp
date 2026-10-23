#include "SceneTexture.h"
#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace isla {
void SceneTexture::reset() {
    for(auto* texture:levels_) SDL_DestroyTexture(texture);
    levels_.clear();width=height=0;
}

void SceneTexture::extract(SDL_Surface* source,int spriteCount) {
    struct Component {int left,top,right,bottom,area;};
    const int w=source->w,h=source->h;
    std::vector<unsigned char> visited(static_cast<std::size_t>(w)*h);
    std::vector<Component> components;
    std::vector<int> queue;
    const auto alpha=[&](int cell) {
        return static_cast<unsigned char*>(source->pixels)[(cell/w)*source->pitch+(cell%w)*4+3];
    };
    for(int start=0;start<w*h;++start) {
        if(visited[start]||alpha(start)<32)continue;
        Component component{start%w,start/w,start%w,start/w,0};
        queue.clear();queue.push_back(start);visited[start]=1;
        for(std::size_t head=0;head<queue.size();++head) {
            const int cell=queue[head],x=cell%w,y=cell/w;
            ++component.area;
            component.left=std::min(component.left,x);component.right=std::max(component.right,x);
            component.top=std::min(component.top,y);component.bottom=std::max(component.bottom,y);
            for(int offset:{1,-1,w,-w}) {
                const int next=cell+offset;
                if(next<0||next>=w*h||std::abs(next%w-x)+std::abs(next/w-y)!=1||visited[next]||alpha(next)<32)continue;
                visited[next]=1;queue.push_back(next);
            }
        }
        if(component.area>256)components.push_back(component);
    }
    std::sort(components.begin(),components.end(),[](const Component& a,const Component& b){return a.area>b.area;});
    if(spriteCount!=4&&spriteCount!=8)throw std::runtime_error("Unsupported sprite atlas layout");
    if(components.size()<static_cast<std::size_t>(spriteCount))throw std::runtime_error("Atlas does not contain the required independent sprites");
    components.resize(spriteCount);
    std::sort(components.begin(),components.end(),[](const Component& a,const Component& b){return a.top+a.bottom<b.top+b.bottom;});
    for(int row=0;row<spriteCount/4;++row)
        std::sort(components.begin()+row*4,components.begin()+row*4+4,[](const Component& a,const Component& b){return a.left<b.left;});
    for(int i=0;i<spriteCount;++i) {
        const auto& c=components[i];
        const int left=std::max(0,c.left-2),top=std::max(0,c.top-2);
        sprites[i]={static_cast<float>(left),static_cast<float>(top),
            static_cast<float>(std::min(w,c.right+3)-left),static_cast<float>(std::min(h,c.bottom+3)-top)};
    }
}

void SceneTexture::build(SDL_Renderer* renderer,SDL_Surface* source) {
    reset();renderer_=renderer;width=source->w;height=source->h;
    for(int y=0;y<height;++y)for(int x=0;x<width;++x) {
        auto* pixel=static_cast<unsigned char*>(source->pixels)+y*source->pitch+x*4;
        for(int channel=0;channel<3;++channel)pixel[channel]=static_cast<unsigned char>((pixel[channel]*pixel[3]+127)/255);
    }
    SDL_Surface* level=source;
    for(;;) {
        SDL_Texture* texture=SDL_CreateTextureFromSurface(renderer,level);
        if(!texture) {if(level!=source)SDL_DestroySurface(level);throw std::runtime_error(SDL_GetError());}
        SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND_PREMULTIPLIED);
        SDL_SetTextureScaleMode(texture,SDL_SCALEMODE_LINEAR);
        levels_.push_back(texture);
        if(level->w<=8||level->h<=8)break;
        SDL_Surface* smaller=SDL_ScaleSurface(level,std::max(1,level->w/2),std::max(1,level->h/2),SDL_SCALEMODE_LINEAR);
        if(level!=source)SDL_DestroySurface(level);
        if(!smaller)throw std::runtime_error(SDL_GetError());
        level=smaller;
    }
    if(level!=source)SDL_DestroySurface(level);
}

void SceneTexture::load(SDL_Renderer* renderer,const std::string& path,int spriteCount) {
    SDL_Surface* input=IMG_Load(path.c_str());
    if(!input)throw std::runtime_error("Cannot load scenery: "+path);
    SDL_Surface* source=SDL_ConvertSurface(input,SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(input);
    if(!source)throw std::runtime_error(SDL_GetError());
    try {if(spriteCount)extract(source,spriteCount);build(renderer,source);}
    catch(...) {SDL_DestroySurface(source);throw;}
    SDL_DestroySurface(source);
}

void SceneTexture::upload(SDL_Renderer* renderer,const unsigned char* rgba,int imageWidth,int imageHeight) {
    SDL_Surface* source=SDL_CreateSurface(imageWidth,imageHeight,SDL_PIXELFORMAT_RGBA32);
    if(!source)throw std::runtime_error(SDL_GetError());
    for(int y=0;y<imageHeight;++y)
        std::memcpy(static_cast<unsigned char*>(source->pixels)+y*source->pitch,rgba+static_cast<std::size_t>(y)*imageWidth*4,imageWidth*4);
    try{build(renderer,source);}catch(...){SDL_DestroySurface(source);throw;}
    SDL_DestroySurface(source);
}

void SceneTexture::draw(ra::Rect destination)const {
    draw({0,0,static_cast<float>(width),static_cast<float>(height)},destination);
}

void SceneTexture::draw(ra::Rect source,ra::Rect destination)const {
    if(levels_.empty()||destination.w<=0||destination.h<=0)return;
    const float ratio=std::min(source.w/destination.w,source.h/destination.h);
    const int mip=std::clamp(static_cast<int>(std::floor(std::log2(std::max(1.0f,ratio)))),0,static_cast<int>(levels_.size()-1));
    const float divisor=static_cast<float>(1<<mip);
    const SDL_FRect src{source.x/divisor,source.y/divisor,source.w/divisor,source.h/divisor};
    const SDL_FRect dst{destination.x,destination.y,destination.w,destination.h};
    SDL_RenderTexture(renderer_,levels_[mip],&src,&dst);
}
}
