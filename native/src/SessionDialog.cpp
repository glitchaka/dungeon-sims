#include "SessionDialog.h"
#include "Multiplayer.h"
#include <ra/Engine.h>
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <filesystem>
#include <random>
#include <limits>
namespace {
struct Field {std::string label,value;float y;std::size_t limit;};
bool ipv4(const std::string& s){int count=0;std::size_t start=0;for(std::size_t i=0;i<=s.size();++i)if(i==s.size()||s[i]=='.'){if(i==start||i-start>3)return false;int n=0;for(auto j=start;j<i;++j){if(s[j]<'0'||s[j]>'9')return false;n=n*10+s[j]-'0';}if(n>255)return false;++count;start=i+1;}return count==4;}
bool menu(ra::Engine& engine,SessionOptions& o,bool connection,const std::string& capture){
 int count=0;auto windows=SDL_GetWindows(&count);auto* window=count?windows[0]:nullptr;SDL_free(windows);if(!window)return false;auto* renderer=SDL_GetRenderer(window);if(!renderer)return false;
 float oldX=1,oldY=1;SDL_GetRenderScale(renderer,&oldX,&oldY);
 auto background=engine.loadTexture((std::filesystem::path(SDL_GetBasePath())/"assets/menu-island.png").string());
 std::array<Field,5> fields{{{"TU NOMBRE",o.name,164,32},{"PARTIDA",o.profile,460,32},{"DIRECCION DE LA OTRA ISLA",o.ip,350,15},{"SEMILLA (VACIA = ALEATORIA)",o.seed?std::to_string(o.seed):"",530,10},{"PUERTO",std::to_string(o.port),600,5}}};
 int action=connection?2:0,focus=connection?2:0;bool selectAll=true,advanced=connection,fresh=o.fresh,done=false,accepted=false;std::string error;int frames=0;
 if(connection){fields[2].y=164;fields[4].y=244;}else{fields[1].y=350;fields[3].y=420;}
 auto visible=[&](int i){return i==0?!connection:i==2?action==2:i==4?advanced&&connection:advanced&&!connection;};
 SDL_StartTextInput(window);
 while(!done){
  int w=0,h=0;SDL_GetWindowSizeInPixels(window,&w,&h);float sx=w/1280.0f,sy=h/800.0f;
  auto hit=[](float x,float y,float a,float b,float width,float height){return x>=a&&x<a+width&&y>=b&&y<b+height;};
  auto submit=[&](){
   error.clear();if(!connection&&(fields[0].value.empty()||fields[0].value.size()>32||fields[0].value.find_first_not_of(' ')==std::string::npos)){error="ESCRIBE TU NOMBRE PARA ENTRAR";focus=0;return;}
   if(action==2&&!ipv4(fields[2].value)){error="REVISA LA DIRECCION: EJEMPLO 192.168.1.20";focus=2;return;}
   if(fields[1].value.empty()||fields[1].value.size()>32||fields[1].value.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_")!=std::string::npos){advanced=true;focus=1;error="LA PARTIDA ADMITE LETRAS, NUMEROS, - Y _";return;}
   unsigned long seed=0,port=0;try{if(fields[4].value.find_first_not_of("0123456789")!=std::string::npos)throw 0;std::size_t used=0;port=std::stoul(fields[4].value,&used);if(used!=fields[4].value.size()||port<1024||port>65535)throw 0;}catch(...){advanced=true;focus=4;error="PUERTO VALIDO: 1024 A 65535";return;}
   if(!fields[3].value.empty())try{if(fields[3].value.find_first_not_of("0123456789")!=std::string::npos)throw 0;std::size_t used=0;seed=std::stoul(fields[3].value,&used);if(used!=fields[3].value.size()||seed==0||seed>std::numeric_limits<unsigned>::max())throw 0;}catch(...){advanced=true;focus=3;error="LA SEMILLA DEBE SER UN NUMERO POSITIVO";return;}
   o.name=fields[0].value;o.profile=fields[1].value;o.host=action==1;o.ip=action==2?fields[2].value:"";o.port=static_cast<unsigned short>(port);o.fresh=fresh;o.seed=fields[3].value.empty()?std::random_device{}():static_cast<unsigned>(seed);
   if(o.profile=="default"&&action!=0)o.profile=action==1?"island-host":"island-guest";
   accepted=done=true;
  };
  SDL_Event e;while(SDL_PollEvent(&e)){
   if(e.type==SDL_EVENT_QUIT){engine.quit();done=true;}
   if(e.type==SDL_EVENT_KEY_DOWN){
    if(e.key.key==SDLK_ESCAPE){done=true;}
    else if(e.key.key==SDLK_RETURN)submit();
    else if(e.key.key==SDLK_TAB){do{focus=(focus+1)%5;}while(!visible(focus));selectAll=true;}
    else if(e.key.key==SDLK_A&&(e.key.mod&SDL_KMOD_CTRL))selectAll=true;
    else if(e.key.key==SDLK_BACKSPACE){auto& value=fields[focus].value;if(selectAll)value.clear();else if(!value.empty()){std::size_t pos=value.size()-1;while(pos>0&&(static_cast<unsigned char>(value[pos])&0xc0)==0x80)--pos;value.erase(pos);}selectAll=false;}
    else if(e.key.key==SDLK_V&&(e.key.mod&SDL_KMOD_CTRL)){char* pasted=SDL_GetClipboardText();if(pasted){auto& value=fields[focus].value;std::string text=pasted;SDL_free(pasted);if(selectAll)value.clear();for(unsigned char c:text)if(c>=32&&value.size()<fields[focus].limit)value+=c;selectAll=false;}}
   }
   if(e.type==SDL_EVENT_TEXT_INPUT){auto& field=fields[focus];if(selectAll)field.value.clear();for(const unsigned char* c=reinterpret_cast<const unsigned char*>(e.text.text?e.text.text:"");*c;++c)if(*c>=32&&field.value.size()<field.limit)field.value+=static_cast<char>(*c);selectAll=false;error.clear();}
   if(e.type==SDL_EVENT_MOUSE_BUTTON_UP&&e.button.button==SDL_BUTTON_LEFT){float x=e.button.x/sx,y=e.button.y/sy;
    for(int i=0;i<5;++i)if(visible(i)&&hit(x,y,814,fields[i].y,390,38)){focus=i;selectAll=true;}
    if(!connection&&hit(x,y,814,250,390,26))fresh=!fresh;
    if(hit(x,y,814,connection?284:300,390,22)){advanced=!advanced;if(!visible(focus)){focus=connection?2:0;selectAll=true;}}
    if(hit(x,y,814,714,390,46))submit();
    if(hit(x,y,814,770,390,22))done=true;
   }
  }
  SDL_SetRenderScale(renderer,sx,sy);engine.clear({11,29,36,255});
  if(background)engine.drawTexture(background,{-80,-40,1040,880});
  engine.fillRect({0,0,790,800},{6,27,32,95});engine.fillRect({770,0,510,800},{14,34,40,250});engine.fillRect({792,36,2,728},{180,153,93,160});
  auto text=[&](float x,float y,const std::string& value,ra::Color c,float size=1){SDL_SetRenderScale(renderer,sx*size,sy*size);engine.drawText(x/size,y/size,value,c);SDL_SetRenderScale(renderer,sx,sy);};
  const ra::Color cream{237,226,196,255},gold{220,184,111,255},muted{155,178,169,255};
  text(64,100,"ISLA VIVA",cream,5);text(68,170,"UN MUNDO QUE VIVE POR SI MISMO",gold,1.5f);
  text(68,651,"CUIDA A TU PUEBLO. INSPIRA SU DESTINO.",cream,1.5f);text(68,683,"CADA ISLA TIENE SU PROPIA HISTORIA.",muted,1.2f);
  text(814,70,connection?"OTRA ISLA TE ESPERA":"TU HISTORIA COMIENZA AQUI",gold,1.5f);
  if(!connection)text(814,112,"CONTINUA TU MUNDO O CREA UNA NUEVA ISLA",muted);
  for(int i=0;i<5;++i)if(visible(i)){const auto& f=fields[i];text(814,f.y-17,f.label,muted);engine.fillRect({814,f.y,390,38},{20,44,49,255});engine.drawRect({814,f.y,390,38},focus==i?gold:ra::Color{68,92,88,255});text(826,f.y+15,f.value,cream,1.1f);if(focus==i&&SDL_GetTicks()%1000<500)engine.fillRect({826+static_cast<float>(f.value.size())*8.8f,f.y+10,1,19},gold);}
  if(action==0&&!connection)text(814,220,"TU ISLA CONTINUA DONDE LA DEJASTE.",muted);
  if(!connection){text(814,259,std::string(fresh?"[X] ":"[ ] ")+"CREAR UNA NUEVA ISLA",fresh?gold:cream);text(814,308,advanced?"OCULTAR OPCIONES DE ISLA":"OPCIONES DE ISLA",muted);}
  else text(814,292,advanced?"OCULTAR OPCIONES":"OPCIONES DE CONEXION",muted);
  if(!error.empty())text(814,681,error,{243,185,123,255});
  engine.fillRect({814,714,390,46},{167,133,73,255});text(836,733,action==2?"CONECTAR CON LA OTRA ISLA":"ENTRAR A MI ISLA",{18,35,39,255},1.4f);text(814,779,connection?"VOLVER A MI ISLA":"SALIR",muted);
  SDL_RenderPresent(renderer);
  if(!capture.empty()&&++frames==3){auto* image=SDL_RenderReadPixels(renderer,nullptr);if(image){SDL_SaveBMP(image,capture.c_str());SDL_DestroySurface(image);}done=true;}
  SDL_Delay(8);
 }
 SDL_StopTextInput(window);engine.destroyTexture(background);SDL_SetRenderScale(renderer,oldX,oldY);return accepted;
}
}
bool chooseSession(ra::Engine& engine,SessionOptions& options,const std::string& capture){return menu(engine,options,false,capture);}
bool chooseConnection(ra::Engine& engine,std::string& ip,unsigned short& port){SessionOptions options;options.ip=ip;options.port=port;if(!menu(engine,options,true,""))return false;ip=options.ip;port=options.port;return true;}
