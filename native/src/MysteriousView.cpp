#include "MysteriousView.h"
#include "World.h"
#include "TerrainProjection.h"
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace isla {
namespace {
void ring(SDL_Renderer* r,float x,float y,float rx,float ry,SDL_Color color) {
    SDL_SetRenderDrawColor(r,color.r,color.g,color.b,color.a);
    for(int i=0;i<64;++i){const float a=i*6.2831853f/64,b=(i+1)*6.2831853f/64;SDL_RenderLine(r,x+std::cos(a)*rx,y+std::sin(a)*ry,x+std::cos(b)*rx,y+std::sin(b)*ry);}
}
void piece(const SceneTexture& texture,ra::Rect source,float x,float y,float width){const float h=width*source.h/source.w;texture.draw(source,{x-width*.5f,y-h,width,h});}
}
void MysteriousView::load(SDL_Renderer* r,const std::filesystem::path& assets){renderer_=r;fish_.load(r,(assets/"sea-fish-v8.png").string(),4);buildWordArt(r);}
void MysteriousView::buildWordArt(SDL_Renderer* renderer) {
    constexpr int width=2048,height=240;
    BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=width;info.bmiHeader.biHeight=-height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
    void* raw=nullptr;HDC dc=CreateCompatibleDC(nullptr);HBITMAP bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&raw,nullptr,0);
    if(!dc||!bitmap){if(bitmap)DeleteObject(bitmap);if(dc)DeleteDC(dc);throw std::runtime_error("Cannot create mystery title");}
    const auto previous=SelectObject(dc,bitmap);std::fill_n(static_cast<unsigned char*>(raw),width*height*4,0);
    const std::wstring text=L"Dios Obrando Misteriosamente";
    HFONT font=CreateFontW(-110,0,0,0,FW_BLACK,TRUE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Georgia");
    const auto oldFont=SelectObject(dc,font);SIZE extent{};GetTextExtentPoint32W(dc,text.c_str(),static_cast<int>(text.size()),&extent);
    if(extent.cx>width-120){SelectObject(dc,oldFont);DeleteObject(font);font=CreateFontW(-static_cast<int>(110.0*(width-120)/extent.cx),0,0,0,FW_BLACK,TRUE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Georgia");SelectObject(dc,font);GetTextExtentPoint32W(dc,text.c_str(),static_cast<int>(text.size()),&extent);}
    SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(255,255,255));
    int x=(width-extent.cx)/2;
    for(wchar_t letter:text){SIZE glyph{};GetTextExtentPoint32W(dc,&letter,1,&glyph);const float phase=static_cast<float>(x+glyph.cx*.5f)/width;const int y=80-static_cast<int>(std::sin(phase*3.14159265f)*35);TextOutW(dc,x,y,&letter,1);x+=glyph.cx;}
    GdiFlush();
    const auto* pixels=static_cast<unsigned char*>(raw);std::vector<unsigned char> rgba(width*height*4);
    const auto mask=[&](int xx,int yy)->unsigned char{return xx<0||yy<0||xx>=width||yy>=height?0:pixels[(yy*width+xx)*4];};
    for(int y=0;y<height;++y)for(int x=0;x<width;++x){auto* pixel=&rgba[(y*width+x)*4];unsigned char a=mask(x,y),edge=a;
        for(int dy=-2;dy<=2;++dy)for(int dx=-2;dx<=2;++dx)edge=std::max(edge,mask(x+dx,y+dy));
        const unsigned char shadow=std::max(mask(x-9,y-10),mask(x-5,y-6));
        if(shadow){pixel[0]=49;pixel[1]=21;pixel[2]=73;pixel[3]=shadow;}
        if(edge){pixel[0]=251;pixel[1]=204;pixel[2]=71;pixel[3]=edge;}
        if(a){const float t=static_cast<float>(x)/width,shine=.8f+.2f*std::cos(y*.07f);pixel[0]=static_cast<unsigned char>((145+100*std::sin(t*6.2831853f+1.2f))*shine);pixel[1]=static_cast<unsigned char>((135+105*std::sin(t*6.2831853f+3.2f))*shine);pixel[2]=static_cast<unsigned char>((145+100*std::sin(t*6.2831853f+5.3f))*shine);pixel[3]=std::max(a,edge);}
    }
    SelectObject(dc,oldFont);DeleteObject(font);SelectObject(dc,previous);DeleteObject(bitmap);DeleteDC(dc);
    wordArt_.upload(renderer,rgba.data(),width,height);
}
void MysteriousView::drawFish(const World& w,float mapX,float mapY,float unit,double time,MysteryTarget held)const {
    for(std::size_t index=0;index<w.seaFish.size();++index){const auto& fish=w.seaFish[index];if(!fish.alive||(held.kind==MysteryKind::Fish&&held.id==static_cast<int>(index)))continue;const int direction=std::abs(std::cos(fish.heading))>std::abs(std::sin(fish.heading))?(std::cos(fish.heading)>0?0:2):(std::sin(fish.heading)>0?1:3);
        const auto source=fish_.sprites[direction];const float scale=unit*.65f/std::max(source.w,source.h);
        const float x=mapX+fish.x*unit,y=mapY+(fish.y-TerrainProjection::elevation(w,fish.x,fish.y))*unit+std::sin(static_cast<float>(time)*4+fish.x)*unit*.025f;
        fish_.draw(source,{x-source.w*scale*.5f,y-source.h*scale*.5f,source.w*scale,source.h*scale});
    }
}
void MysteriousView::drawHeld(ra::Engine& engine,const World& w,MysteryTarget target,float x,float y,float unit,double time,const MysteryArt& art,bool validDestination)const {
    SDL_SetRenderDrawBlendMode(renderer_,SDL_BLENDMODE_BLEND);
    const auto tint=validDestination?SDL_Color{183,232,255,220}:SDL_Color{244,100,101,220};
    for(int i=0;i<3;++i)ring(renderer_,x,y,unit*(.5f+i*.04f),unit*(.18f+i*.02f),tint);
    const SceneTexture* texture=nullptr;ra::Rect source{};float width=unit;const int id=target.id;
    switch(target.kind){
    case MysteryKind::Resident:if(id>=0&&id<static_cast<int>(w.residents.size()))art.people.draw(w.residents[id],w.elapsed,x,y-12,unit,-1,time);break;
    case MysteryKind::Animal:if(id>=0&&id<static_cast<int>(w.animals.size()))engine.drawTextureRegion(art.animals,{static_cast<float>(w.animals[id].kind*8),32,8,8},{x-unit*.5f,y-unit-12,unit,unit});break;
    case MysteryKind::Fish:texture=&fish_;source=fish_.sprites[0];width=unit*.7f;break;
    case MysteryKind::Boat:if(id>=0&&id<static_cast<int>(w.ships.size()))art.people.drawEmptyBoat(w.ships[id],time,x,y-unit*.4f,unit);break;
    case MysteryKind::Prop:for(const auto& p:w.props)if(p.uid==id){
        if(p.tree){texture=&art.nature;source=art.natureRegions[0];width=unit*2.6f;}
        else if(p.kind=="crystal"||p.kind=="townhall"||p.kind=="altar"||p.kind=="dock"){texture=&art.civic;source=art.civicRegions[p.kind=="townhall"?0:p.kind=="altar"?1:p.kind=="dock"?2:3];width=unit*2.8f;}
        else {texture=&art.buildings;source=art.buildingRegions[p.uid%4];width=unit*2.6f;}break;}break;
    case MysteryKind::Crop:for(const auto& c:w.crops)if(c.cell==id){texture=&art.nature;source=art.natureRegions[c.stage==0?5:c.stage==3?4:7];width=unit*1.2f;break;}break;
    case MysteryKind::Camp:texture=&art.buildings;source=art.buildingRegions[0];width=unit*1.6f;break;
    case MysteryKind::Gift:engine.fillRect({x-unit*.25f,y-unit*.5f-12,unit*.5f,unit*.5f},{235,196,69,255});break;
    case MysteryKind::Water:SDL_SetRenderDrawBlendMode(renderer_,SDL_BLENDMODE_BLEND);for(int i=0;i<12;++i)ring(renderer_,x,y-unit*.45f,unit*(.08f+i*.02f),unit*(.2f+i*.02f),{63,198,239,210});break;
    case MysteryKind::Road:engine.fillRect({x-unit*.4f,y-unit*.2f-12,unit*.8f,unit*.3f},{178,145,98,255});break;
    default:break;
    }
    if(texture)piece(*texture,source,x,y-12,width);
}
void MysteriousView::drawEffect(SDL_Renderer* r,const World& w,ra::Rect viewport,float mapX,float mapY,float unit,double now)const {
    const float age=static_cast<float>(now-effectAt_);if(effectCell_<0||age<0||age>3.5f)return;
    const float fade=std::min(1.0f,(3.5f-age)*2),x=mapX+(effectCell_%mapWidth+.5f)*unit,y=mapY+(effectCell_/mapWidth+.5f-TerrainProjection::elevation(w,effectCell_%mapWidth+.5f,effectCell_/mapWidth+.5f))*unit;
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
    for(int i=0;i<5;++i)ring(r,x,y,(.4f+age*.6f+i*.1f)*unit,(.16f+age*.2f+i*.035f)*unit,{245,213,111,static_cast<unsigned char>((135-i*20)*fade)});
    for(int i=0;i<28;++i){const float angle=i*2.399963f+age,rad=(.4f+std::fmod(i*.17f+age*.5f,1.4f))*unit;const float px=x+std::cos(angle)*rad,py=y-std::abs(std::sin(angle))*rad-age*unit*.2f;SDL_SetRenderDrawColor(r,255,231,157,static_cast<unsigned char>(220*fade));SDL_RenderLine(r,px-2,py,px+2,py);SDL_RenderLine(r,px,py-2,px,py+2);}
    const float width=std::min(viewport.w*.94f,1100.0f),height=width*wordArt_.height/wordArt_.width;
    wordArt_.draw({viewport.x+(viewport.w-width)*.5f,viewport.y+35-std::sin(age*3)*4,width,height});
}
}
