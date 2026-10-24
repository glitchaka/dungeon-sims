#include "ResidentSprites.h"
#include "World.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>

int main(int argc,char** argv) {
    SDL_Surface* surface=nullptr;
    SDL_Renderer* renderer=nullptr;
    try {
        if(argc!=2)throw std::runtime_error("Expected asset directory");
        surface=SDL_CreateSurface(100,100,SDL_PIXELFORMAT_RGBA32);
        renderer=SDL_CreateSoftwareRenderer(surface);
        if(!renderer)throw std::runtime_error(SDL_GetError());
        {
            isla::ResidentSprites sprites;sprites.load(renderer,argv[1]);
            for(double age:{0.0,2159.0,6479.0,6480.0,23400.0})
            for(bool female:{false,true})for(int direction=0;direction<4;++direction)
            for(double elapsed:{0.0,.2}) {
                SDL_SetRenderDrawColor(renderer,0,0,0,0);SDL_RenderClear(renderer);
                isla::Resident resident;resident.ageDays=age;resident.female=female;
                resident.x=10.5f;resident.y=10.5f;
                const int offsets[]={isla::mapWidth,1,-isla::mapWidth,-1};
                resident.path={10*isla::mapWidth+10+offsets[direction]};
                sprites.draw(resident,elapsed,50,90,48);SDL_RenderPresent(renderer);
                int left=100,right=-1,top=100,bottom=-1;
                for(int y=0;y<100;++y)for(int x=0;x<100;++x) {
                    const auto* pixel=static_cast<unsigned char*>(surface->pixels)+y*surface->pitch+x*4;
                    if(pixel[3]<32)continue;
                    left=std::min(left,x);right=std::max(right,x);
                    top=std::min(top,y);bottom=std::max(bottom,y);
                }
                if(right<left||left<10||right>90||top<39||bottom<85||bottom>90)
                    throw std::runtime_error("Character missing, clipped, oversized or incorrectly anchored");
            }
            auto fingerprint=[&]() {
                SDL_RenderPresent(renderer);std::uint64_t hash=1469598103934665603ull;int visible=0;
                for(int y=0;y<100;++y)for(int x=0;x<100;++x){
                    const auto* pixel=static_cast<unsigned char*>(surface->pixels)+y*surface->pitch+x*4;
                    if(pixel[3]>=32)++visible;
                    for(int c=0;c<4;++c)hash=(hash^pixel[c])*1099511628211ull;
                }
                if(visible<20)throw std::runtime_error("Action frame is empty");return hash;
            };
            auto clear=[&](){SDL_SetRenderDrawColor(renderer,0,0,0,0);SDL_RenderClear(renderer);};
            for(bool female:{false,true})for(auto task:{isla::Task::Till,isla::Task::Gather}) {
                isla::Resident worker;worker.female=female;worker.task=task;
                clear();sprites.draw(worker,100,50,90,32);const auto first=fingerprint();
                worker.actionTimer=.4f;clear();sprites.draw(worker,100,50,90,32);
                if(fingerprint()==first)throw std::runtime_error("Working animation has identical frames");
                worker.actionTimer=0;
                clear();sprites.draw(worker,100,50,90,32,-1,0);const auto visualFirst=fingerprint();
                clear();sprites.draw(worker,100,50,90,32,-1,.4);
                if(fingerprint()==visualFirst)throw std::runtime_error("Work rendering ignores independent animation time");
            }
            for(bool female:{false,true})for(int direction=0;direction<4;++direction) {
                isla::Resident rower;rower.female=female;isla::Ship boat;boat.state="outbound";boat.x=boat.y=10.5f;
                const int offsets[]={isla::mapWidth,1,-isla::mapWidth,-1};boat.path={10*isla::mapWidth+10+offsets[direction]};
                clear();sprites.drawBoat(boat,rower,0,50,50,28);const auto first=fingerprint();
                clear();sprites.drawBoat(boat,rower,.4,50,50,28);
                if(fingerprint()==first)throw std::runtime_error("Rowing animation has identical frames");
            }
        }
        SDL_DestroyRenderer(renderer);SDL_DestroySurface(surface);
        std::cout<<"All character atlases render at childhood/adulthood boundaries, four directions and both steps\n";
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<e.what()<<'\n';
        if(renderer)SDL_DestroyRenderer(renderer);if(surface)SDL_DestroySurface(surface);
        return 1;
    }
}

