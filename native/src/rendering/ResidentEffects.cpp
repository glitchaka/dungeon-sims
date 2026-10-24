#include "ResidentEffects.h"
#include <algorithm>
#include <cmath>

namespace isla {
namespace {
void ellipse(SDL_Renderer* renderer,float x,float y,float rx,float ry,SDL_Color color) {
    SDL_SetRenderDrawColor(renderer,color.r,color.g,color.b,color.a);
    for(int dy=-static_cast<int>(std::ceil(ry));dy<=static_cast<int>(std::ceil(ry));++dy) {
        const float fraction=static_cast<float>(dy)/std::max(1.0f,ry);
        if(std::abs(fraction)>1)continue;
        const float half=rx*std::sqrt(1-fraction*fraction);
        SDL_RenderLine(renderer,x-half,y+dy,x+half,y+dy);
    }
}
}
void ResidentEffects::heart(SDL_Renderer* r,float x,float y,float size) {
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
    ellipse(r,x-size*.22f,y,size*.3f,size*.3f,{190,64,65,235});
    ellipse(r,x+size*.22f,y,size*.3f,size*.3f,{190,64,65,235});
    const SDL_FColor color{.75f,.25f,.25f,.92f};
    SDL_Vertex vertices[]={{{x-size*.49f,y},{color},{0,0}},{{x+size*.49f,y},{color},{0,0}},{{x,y+size*.65f},{color},{0,0}}};
    SDL_RenderGeometry(r,nullptr,vertices,3,nullptr,0);
}
void ResidentEffects::surprise(SDL_Renderer* r,float x,float y,float size) {
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r,245,218,146,255);
    SDL_FRect stem{x-size*.09f,y-size*.5f,std::max(1.0f,size*.18f),size*.65f};
    SDL_RenderFillRect(r,&stem);ellipse(r,x,y+size*.4f,size*.1f,size*.1f,{245,218,146,255});
}
void ResidentEffects::impact(SDL_Renderer* r,float x,float y,float size,double phase,bool wood) {
    const float t=std::fmod(static_cast<float>(phase)*1.5f,1.0f);
    if(t<.45f)return;
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
    for(int i=0;i<4;++i) {
        const float travel=(t-.45f)*size;
        ellipse(r,x+(i-1.5f)*travel*.55f,y-travel*(.4f+(i%2)*.3f),std::max(1.0f,size*.035f),std::max(1.0f,size*.02f),
            wood?SDL_Color{186,143,85,180}:SDL_Color{113,83,43,160});
    }
}
}


