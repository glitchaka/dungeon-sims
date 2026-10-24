#include "SessionDialog.h"
#include <ra/Engine.h>
#include <SDL3/SDL.h>
#include <iostream>
#include <stdexcept>
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void text(const char* value){SDL_Event e{};e.type=SDL_EVENT_TEXT_INPUT;e.text.text=value;SDL_PushEvent(&e);}
void key(SDL_Keycode value,SDL_Keymod mod=SDL_KMOD_NONE){SDL_Event e{};e.type=SDL_EVENT_KEY_DOWN;e.key.key=value;e.key.mod=mod;SDL_PushEvent(&e);}
void click(float x,float y){SDL_Event e{};e.type=SDL_EVENT_MOUSE_BUTTON_UP;e.button.button=SDL_BUTTON_LEFT;e.button.x=x;e.button.y=y;SDL_PushEvent(&e);}
int main(){try{
 ra::EngineConfig config;config.width=1280;config.height=800;config.vsync=false;ra::Engine engine(config);
 SessionOptions options;options.host=true;options.ip="127.0.0.1";text("Astrum");key(SDLK_RETURN);check(chooseSession(engine,options)&&options.name=="Astrum"&&!options.host&&options.ip.empty(),"Start screen must only enter the island, not select a connection role");
 SessionOptions cancelled;key(SDLK_ESCAPE);check(!chooseSession(engine,cancelled),"Escape accepted an unwanted session");
 std::string ip;unsigned short port=32145;text("999.1.1.1");key(SDLK_RETURN);key(SDLK_A,SDL_KMOD_CTRL);text("10.0.0.4");key(SDLK_TAB);text("40000");key(SDLK_RETURN);check(chooseConnection(engine,ip,port)&&ip=="10.0.0.4"&&port==40000,"In-game connection menu lost address or custom port");
 std::cout<<"Integrated menu: text entry, invalid IP, connection action, custom port and cancellation passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

