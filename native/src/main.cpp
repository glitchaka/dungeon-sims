#include <ra/Engine.h>
#include <SDL3/SDL.h>
#include "World.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
ra::Color color(isla::cnc::Color c) { return {c.r,c.g,c.b,c.a}; }
class IslaVivaGame final : public ra::Game {
public:
    explicit IslaVivaGame(bool smoke, std::filesystem::path assets, std::string capture): assetRoot_(std::move(assets)), capturePath_(std::move(capture)), raining_(smoke), smoke_(smoke) {}
    void onStart(ra::Engine& engine) override {
        engine.setWindowTitle("Isla Viva | Simulacion nativa");
        terrain_=engine.loadTexture((assetRoot_/"geology.png").string());
        natureTexture_=engine.loadTexture((assetRoot_/"nature-pieces.png").string());
        buildingsTexture_=engine.loadTexture((assetRoot_/"building-pieces.png").string());
        std::ifstream pieceFile(assetRoot_/"pieces.json");auto pieceData=nlohmann::json::parse(pieceFile);
        for(int i=0;i<8;++i){const auto& n=pieceData.at("nature").at(i);const auto& b=pieceData.at("buildings").at(i);natureRegions_[i]={n[0].get<float>(),n[1].get<float>(),n[2].get<float>(),n[3].get<float>()};buildingRegions_[i]={b[0].get<float>(),b[1].get<float>(),b[2].get<float>(),b[3].get<float>()};}
        residentsTexture_=engine.loadTexture((assetRoot_/"residents.png").string());
        fieldsTexture_=engine.loadTexture((assetRoot_/"fields.png").string());
        houseTexture_=engine.loadTexture((assetRoot_/"house.png").string());
        roadTexture_=engine.loadTexture((assetRoot_/"road-tiles.png").string());
        objectsTexture_=engine.loadTexture((assetRoot_/"objects.png").string());
        if(!terrain_ || !residentsTexture_ || !fieldsTexture_ || !objectsTexture_ || !houseTexture_ || !roadTexture_ || !natureTexture_ || !buildingsTexture_) throw std::runtime_error("Faltan recursos del mapa en la carpeta assets");
        if(!smoke_&&world_.load(savePath()))message_="Partida recuperada";
        world_.updateFishing(0);
        updateTerrain(engine);
        int count=0; SDL_Window** windows=SDL_GetWindows(&count);
        renderer_=count>0?SDL_GetRenderer(windows[0]):nullptr; SDL_free(windows);
        if(!renderer_) throw std::runtime_error("Cannot access map renderer");
    }
    void onStop(ra::Engine& engine) override { engine.destroyTexture(terrain_); engine.destroyTexture(residentsTexture_); engine.destroyTexture(fieldsTexture_); engine.destroyTexture(objectsTexture_); engine.destroyTexture(houseTexture_); engine.destroyTexture(roadTexture_); engine.destroyTexture(natureTexture_); engine.destroyTexture(buildingsTexture_); if(!smoke_) world_.save(savePath()); }
    void onUpdate(ra::Engine& engine,float delta) override {
        if (engine.keyPressed(ra::Key::Escape)) { if(!tool_.empty()){tool_.clear();relocateSource_=-1;message_="Orden de construccion cancelada";}else engine.quit(); return; }
        if (smoke_ && ++frames_>=180) { engine.quit(); return; }
        if (engine.keyPressed(ra::Key::Space)) paused_=!paused_;
        if (engine.keyPressed(ra::Key::E)) speed_=speed_==1 ? 4 : speed_==4 ? 12 : 1;
        if (engine.keyPressed(ra::Key::Q)) raining_=!raining_;
        layout(engine);
        const bool overMap=engine.mouseX()>=20 && engine.mouseX()<engine.windowWidth()-352
            && engine.mouseY()>=20 && engine.mouseY()<engine.windowHeight()-20;
        if(overMap && engine.mouseWheelY()!=0) {
            const float worldX=(engine.mouseX()-mapX_)/unit_,worldY=(engine.mouseY()-mapY_)/unit_;
            zoom_=std::clamp(zoom_+engine.mouseWheelY()*.1f,1.0f,2.0f);
            layout(engine);
            centerX_=worldX-(engine.mouseX()-viewportCenterX_)/unit_;
            centerY_=worldY-(engine.mouseY()-viewportCenterY_)/unit_;
            layout(engine);
        }
        if(overMap && engine.mousePressed(ra::MouseButton::Middle)) {
            dragging_=true;lastMouseX_=engine.mouseX();lastMouseY_=engine.mouseY();
        }
        if(!engine.mouseDown(ra::MouseButton::Middle)) dragging_=false;
        if(dragging_) {
            centerX_-=(engine.mouseX()-lastMouseX_)/unit_;
            centerY_-=(engine.mouseY()-lastMouseY_)/unit_;
            lastMouseX_=engine.mouseX();lastMouseY_=engine.mouseY();
        }
        const float panStep=std::clamp(delta,0.0f,.1f)*400/unit_;
        if(engine.keyDown(ra::Key::A)) centerX_-=panStep;
        if(engine.keyDown(ra::Key::D)) centerX_+=panStep;
        if(engine.keyDown(ra::Key::W)) centerY_-=panStep;
        if(engine.keyDown(ra::Key::S)) centerY_+=panStep;
        if(engine.keyPressed(ra::Key::Enter)) {zoom_=1;centerX_=64;centerY_=48;}
        layout(engine);
        if(engine.mousePressed(ra::MouseButton::Left) && !overMap) for(const auto& control:controls_) if(engine.mouseX()>=control.rect.x && engine.mouseX()<control.rect.x+control.rect.w && engine.mouseY()>=control.rect.y && engine.mouseY()<control.rect.y+control.rect.h){activate(control.id);break;}
        if (overMap && engine.mousePressed(ra::MouseButton::Left)) {
            const float x=(engine.mouseX()-mapX_)/unit_, y=(engine.mouseY()-mapY_)/unit_;
            if(x>=0&&y>=0&&x<isla::mapWidth&&y<isla::mapHeight) {
                const int cell=isla::cellId(static_cast<int>(x),static_cast<int>(y));
                if(!tool_.empty()) {
                    const auto jobsBefore=world_.jobs.size();
                    if(tool_=="relocate") {
                        if(relocateSource_<0){relocateSource_=cell;message_="Ahora elige el destino de la reubicacion";}
                        else{message_=world_.relocate(relocateSource_,cell);relocateSource_=-1;}
                    } else message_=world_.designate(tool_,cell);
                    if(world_.jobs.size()>jobsBefore) message_=world_.orderUnit(static_cast<int>(selected_),"job",world_.jobs.back().id);
                } else {
                    float nearest=2;selectedProp_=-1;
                    for(std::size_t i=0;i<world_.residents.size();++i){const auto& person=world_.residents[i];if(!person.alive||person.inDungeon||person.inBoat)continue;float distance=std::hypot(x-person.x,y-person.y);if(distance<nearest){nearest=distance;selected_=i;}}
                    if(auto* object=world_.propAt(cell)) selectedProp_=object->uid;
                }
            }
        }
        if(overMap && engine.mousePressed(ra::MouseButton::Right)) {
            const float x=(engine.mouseX()-mapX_)/unit_,y=(engine.mouseY()-mapY_)/unit_;
            if(x>=0&&y>=0&&x<isla::mapWidth&&y<isla::mapHeight) message_=world_.orderUnit(static_cast<int>(selected_),"work",isla::cellId(static_cast<int>(x),static_cast<int>(y)));
        }
        if(!smoke_){autosaveClock_+=delta;if(autosaveClock_>=45){autosaveClock_=0;if(!world_.save(savePath()))message_="No se pudo guardar la partida";}}
        if (paused_) return;
        accumulator_+=std::clamp(delta,0.0f,.25f)*static_cast<float>(speed_);
        while(accumulator_>=.1f) { world_.tick(.1f); accumulator_-=.1f; }
        const int phase=static_cast<int>(std::fmod(world_.elapsed,300.0f)/75);
        if (phase!=phase_) updateTerrain(engine);
    }
    void onRender(ra::Engine& engine) override {
        layout(engine);
        engine.clear({14,25,31,255});
        const SDL_Rect mapClip{20,20,std::max(1,engine.windowWidth()-372),std::max(1,engine.windowHeight()-40)};
        SDL_SetRenderClipRect(renderer_,&mapClip);
        drawTerrain(engine);
        const auto gold=color(isla::cnc::residentColors[0]);
        // Every visible cell and object is drawn from the current mutable world state.
        for(const auto& crop:world_.crops) {
            const float x=static_cast<float>(crop.cell%isla::mapWidth), y=static_cast<float>(crop.cell/isla::mapWidth);
            const int variant=crop.stage==0?5:crop.stage==3?4:7;
            drawPiece(engine,natureTexture_,natureRegions_[variant],x+.5f,y+1.0f,1.2f);
        }
        drawEntities(engine);
        for(const auto& job:world_.jobs)engine.drawRect({mapX_+(job.cell%isla::mapWidth)*unit_,mapY_+(job.cell/isla::mapWidth)*unit_,unit_,unit_},{249,186,78,255});
        if(!tool_.empty()) {
            const int x=static_cast<int>(std::floor((engine.mouseX()-mapX_)/unit_)),y=static_cast<int>(std::floor((engine.mouseY()-mapY_)/unit_));
            if(x>=0&&y>=0&&x<isla::mapWidth&&y<isla::mapHeight)engine.drawRect({mapX_+x*unit_,mapY_+y*unit_,unit_*(tool_=="house"?2:1),unit_*(tool_=="house"?2:1)},{255,222,123,255});
        }
        if(world_.night()) engine.fillRect({mapX_,mapY_,isla::mapWidth*unit_,isla::mapHeight*unit_},{12,26,62,90});
        if(world_.alarmSeconds>0) {
            const float x=mapX_+world_.alarmX*unit_, y=mapY_+world_.alarmY*unit_;
            engine.drawLine(x-6,y-6,x+6,y+6,{240,80,64,255});
            engine.drawLine(x-6,y+6,x+6,y-6,{240,80,64,255});
        }
        if(raining_) {
            const int count=std::min(300,static_cast<int>(isla::cnc::rainDensity/4));
            for(int i=0;i<count;++i) {
                const float rainSpeed=isla::cnc::rainMinSpeed+(isla::cnc::rainMaxSpeed-isla::cnc::rainMinSpeed)*static_cast<float>(i%17)/16;
                const float x=mapX_+std::fmod(i*17.13f+world_.elapsed*2,isla::mapWidth*unit_);
                const float y=mapY_+std::fmod(i*11.71f+world_.elapsed*rainSpeed*unit_,isla::mapHeight*unit_);
                const float alpha=isla::cnc::rainMinAlpha+(isla::cnc::rainMaxAlpha-isla::cnc::rainMinAlpha)*static_cast<float>(i%7)/6;
                engine.drawLine(x,y,x-2,y+5,{160,197,220,static_cast<unsigned char>(alpha*255)});
            }
        }
        SDL_SetRenderClipRect(renderer_,nullptr);
        const float panel=std::max(12.0f,static_cast<float>(engine.windowWidth())-324);
        engine.fillRect({panel,16,308,static_cast<float>(engine.windowHeight())-32},{28,42,48,255});
        float y=34;
        auto text=[&](const std::string& value,ra::Color c) {engine.drawText(panel+14,y,value,c);y+=20;};
        const ra::Color white{214,225,212,255};
        text("ISLA VIVA",gold);
        text("DIA "+std::to_string(static_cast<int>(world_.elapsed/300)+1)+"  "+std::to_string(speed_)+"X",white);
        constexpr const char* times[]{"MANANA","TARDE","ATARDECER","NOCHE"};
        text(times[phase_],white);
        text(paused_?"PAUSA / ESPACIO":"ESPACIO: PAUSA",white);
        text("E: VELOCIDAD  Q: LLUVIA",white);
        text("RUEDA: ZOOM "+std::to_string(static_cast<int>(std::round(zoom_*100)))+"%",white);
        text("CENTRAL / WASD: MOVER",white);
        text("ENTER: VER ISLA ENTERA",white);
        text("CLIC: INSPECCIONAR",white);
        text("CLIC DERECHO: MOVER / TRABAJAR",white);
        y+=10;
        const auto& r=world_.residents[selected_];
        text(r.name,color(isla::cnc::residentColors[selected_%isla::cnc::residentColors.size()]));
        text(isla::World::taskName(r.task),white);
        text("COMIDA "+std::to_string(static_cast<int>(r.hunger))+"  AGUA "+std::to_string(static_cast<int>(r.thirst)),white);
        text("ENERGIA "+std::to_string(static_cast<int>(r.energy)),white);
        text("SOCIAL "+std::to_string(static_cast<int>(r.social))+"  DIVERSION "+std::to_string(static_cast<int>(r.fun)),white);
        text("HIGIENE "+std::to_string(static_cast<int>(r.hygiene)),white);
        text("SALUD "+std::to_string(static_cast<int>(r.health))+" / "+std::to_string(static_cast<int>(isla::cnc::maxHealth)),white);
        text("GRANO "+std::to_string(world_.grain),gold);
        y+=10;
        text("LA ALDEA",gold);
        text(std::to_string(std::count_if(world_.residents.begin(),world_.residents.end(),[](const isla::Resident& p){return p.alive;}))+" HABITANTES VIVOS",white);
        text("MADERA "+std::to_string(world_.communityWood)+" MONEDAS "+std::to_string(world_.treasury),white);
        text("PESCADO "+std::to_string(r.fish),white);
        text("COSECHAS "+std::to_string(world_.harvests),white);
        text(raining_ ? "ESTA LLOVIENDO" : "CIELO DESPEJADO",white);

        controls_.clear();float buttonY=y+8;int column=0;
        auto button=[&](const std::string& id,const std::string& title){
            const ra::Rect rectangle{panel+12+column*144.0f,buttonY,138,24};
            engine.fillRect(rectangle,tool_==id?ra::Color{86,77,44,255}:ra::Color{46,66,73,255});
            engine.drawText(rectangle.x+5,rectangle.y+8,title,white);controls_.push_back({rectangle,id});
            if(++column==2){column=0;buttonY+=28;}
        };
        button("select","INSPECCIONAR");button("stop","CANCELAR ORDEN");
        button("house","CONSTRUIR CASA");button("farm","NUEVO CAMPO");
        button("road","CAMINO / PUENTE");button("demolish","DEMOLER / TALAr");
        button("relocate","REUBICAR");button("protect","PROTEGER ZONA");
        button("fish0","PESCAR OESTE");button("fish1","PESCAR ESTE");
        button("save","GUARDAR");button("load","CARGAR");
        if(!tool_.empty())engine.drawText(panel+12,buttonY+8,"HERRAMIENTA: "+tool_,gold);
        const float statusY=static_cast<float>(engine.windowHeight())-62;
        engine.fillRect({panel+6,statusY-4,296,48},{20,33,38,255});
        const std::string status=message_.empty()?"Selecciona unidad y da una orden":message_;
        for(std::size_t i=0;i<status.size()&&i<102;i+=34)engine.drawText(panel+12,statusY+static_cast<float>(i/34)*12,status.substr(i,34),gold);
        if(!capturePath_.empty() && frames_==2) {
            int count=0;
            SDL_Window** windows=SDL_GetWindows(&count);
            SDL_Surface* surface=count>0?SDL_RenderReadPixels(SDL_GetRenderer(windows[0]),nullptr):nullptr;
            SDL_free(windows);
            if(!surface) throw std::runtime_error("Cannot capture map preview");
            const bool saved=SDL_SaveBMP(surface,capturePath_.c_str());
            SDL_DestroySurface(surface);
            if(!saved) throw std::runtime_error("Cannot save map preview");
        }
    }
private:
    struct Control { ra::Rect rect; std::string id; };
    std::vector<Control> controls_;
    std::string tool_,message_;int relocateSource_{-1};float autosaveClock_{};
    std::string savePath() const {
        const char* local=std::getenv("LOCALAPPDATA");
        return ((local?std::filesystem::path(local):assetRoot_.parent_path())/"IslaVivaNative"/"world-v6.json").string();
    }
    void activate(const std::string& id) {
        if(id=="select"){tool_.clear();relocateSource_=-1;message_="Modo inspeccion";}
        else if(id=="stop") message_=world_.orderUnit(static_cast<int>(selected_),"stop",0);
        else if(id=="fish0"||id=="fish1") message_=world_.startFishing(static_cast<int>(selected_),id=="fish0"?0:1);
        else if(id=="save")message_=world_.save(savePath())?"Partida guardada":"Error al guardar";
        else if(id=="load"){message_=world_.load(savePath())?"Partida cargada":"No hay un guardado compatible";selected_=std::min(selected_,world_.residents.size()-1);selectedProp_=-1;}
        else{tool_=id;relocateSource_=-1;message_=id=="relocate"?"Elige primero el origen y despues el destino":"Elige una celda para el encargo";}
    }
    void drawPiece(ra::Engine& engine,const ra::Texture& texture,ra::Rect source,float x,float footY,float width) {
        const float height=width*source.h/std::max(1.0f,source.w);
        engine.drawTextureRegion(texture,source,{mapX_+(x-width*.5f)*unit_,mapY_+(footY-height)*unit_,width*unit_,height*unit_});
    }
    void drawEntities(ra::Engine& engine) {
        struct Item {float bottom;int kind;std::size_t index;};std::vector<Item> draw;
        for(std::size_t i=0;i<world_.props.size();++i)draw.push_back({world_.props[i].y+world_.props[i].halfDepth,0,i});
        for(std::size_t i=0;i<world_.animals.size();++i)if(world_.animals[i].alive)draw.push_back({world_.animals[i].y+.2f,1,i});
        for(std::size_t i=0;i<world_.residents.size();++i){const auto& r=world_.residents[i];if(r.alive&&!r.inDungeon&&!r.inBoat)draw.push_back({r.y+.2f,2,i});}
        for(std::size_t i=0;i<world_.camps.size();++i)draw.push_back({static_cast<float>(world_.camps[i].cell/isla::mapWidth)+1,3,i});
        std::sort(draw.begin(),draw.end(),[](const Item& a,const Item& b){return a.bottom<b.bottom;});
        for(const auto& item:draw){
            if(item.kind==0){const auto& p=world_.props[item.index];if(p.tree)drawPiece(engine,natureTexture_,natureRegions_[p.health<40?3:p.uid%2],p.x+static_cast<float>((p.uid*17)%7-3)*.045f,p.y+.5f,2.6f);else drawPiece(engine,buildingsTexture_,buildingRegions_[p.uid%4],p.x,p.y+1,2.6f);}
            else if(item.kind==1){const auto& a=world_.animals[item.index];engine.drawTextureRegion(residentsTexture_,{static_cast<float>(a.kind*8),32,8,8},{mapX_+(a.x-.5f)*unit_,mapY_+(a.y-.5f)*unit_,unit_,unit_});}
            else if(item.kind==2){const auto& r=world_.residents[item.index];int direction=0;if(r.next<r.path.size()){int next=r.path[r.next];float dx=next%isla::mapWidth+.5f-r.x,dy=next/isla::mapWidth+.5f-r.y;direction=std::abs(dx)>std::abs(dy)?(dx>0?1:3):(dy>0?0:2);}int step=r.next<r.path.size()?static_cast<int>(world_.elapsed*6)%4:0;int sx=(direction==1||direction==3?1:0)+(step%2)*2,sy=(direction==2||direction==3?1:0)+(step/2)*2;engine.drawTextureRegion(residentsTexture_,{static_cast<float>(sx*8),static_cast<float>(sy*8),8,8},{mapX_+(r.x-.6f)*unit_,mapY_+(r.y-.8f)*unit_,unit_*1.2f,unit_*1.2f});if(item.index==selected_)engine.drawRect({mapX_+(r.x-.5f)*unit_,mapY_+(r.y-.5f)*unit_,unit_,unit_},{249,206,122,255});}
            else{const auto& camp=world_.camps[item.index];drawPiece(engine,buildingsTexture_,buildingRegions_[0],camp.cell%isla::mapWidth+.5f,camp.cell/isla::mapWidth+1.0f,1.6f);}
        }
        for(const auto& ship:world_.ships){if(ship.seaHome<0)continue;drawPiece(engine,buildingsTexture_,buildingRegions_[4],ship.cell%isla::mapWidth+.5f,ship.cell/isla::mapWidth+1.2f,2.6f);if(ship.state!="lost")drawPiece(engine,buildingsTexture_,buildingRegions_[6],ship.x,ship.y+.7f,1.7f);}
    }
    void drawTerrain(ra::Engine& engine) {
        engine.drawTexture(terrain_,{mapX_,mapY_,isla::mapWidth*unit_,isla::mapHeight*unit_});
        auto road=[&](int x,int y){auto t=world_.terrainAt(x,y);return t==isla::Terrain::Road||t==isla::Terrain::Bridge;};
        const int left=std::max(0,static_cast<int>(std::floor((20-mapX_)/unit_))),right=std::min(isla::mapWidth,static_cast<int>(std::ceil((engine.windowWidth()-352-mapX_)/unit_)));
        const int top=std::max(0,static_cast<int>(std::floor((20-mapY_)/unit_))),bottom=std::min(isla::mapHeight,static_cast<int>(std::ceil((engine.windowHeight()-20-mapY_)/unit_)));
        const int radius=std::max(1,static_cast<int>(unit_*.16f));
        for(int y=top;y<bottom;++y)for(int x=left;x<right;++x){auto type=world_.terrainAt(x,y);if(type==isla::Terrain::Bridge){drawPiece(engine,buildingsTexture_,buildingRegions_[7],x+.5f,y+1.0f,1.8f);continue;}if(type!=isla::Terrain::Road)continue;float cx=mapX_+(x+.5f)*unit_,cy=mapY_+(y+.5f)*unit_;for(int yy=-radius;yy<=radius;++yy){int half=static_cast<int>(std::sqrt(static_cast<float>(radius*radius-yy*yy)));engine.drawLine(cx-half,cy+yy,cx+half,cy+yy,{194,169,116,255});}for(auto step:std::array<std::array<int,2>,2>{{{1,0},{0,1}}})if(road(x+step[0],y+step[1]))for(int offset=-radius;offset<=radius;++offset)engine.drawLine(cx+(step[1]?offset:0),cy+(step[0]?offset:0),cx+step[0]*unit_+(step[1]?offset:0),cy+step[1]*unit_+(step[0]?offset:0),{194,169,116,255});}
    }
    void box(ra::Engine& engine,float x,float y,float w,float h,ra::Color c) const {
        engine.fillRect({mapX_+x*unit_,mapY_+y*unit_,w*unit_,h*unit_},c);
    }
    void layout(ra::Engine& engine) {
        const float viewportWidth=std::max(1.0f,static_cast<float>(engine.windowWidth())-372);
        const float viewportHeight=std::max(1.0f,static_cast<float>(engine.windowHeight())-40);
        viewportCenterX_=20+viewportWidth*.5f;viewportCenterY_=20+viewportHeight*.5f;
        unit_=std::max(.01f,std::min(viewportWidth/isla::mapWidth,viewportHeight/isla::mapHeight))*zoom_;
        const float halfWidth=viewportWidth/(2*unit_),halfHeight=viewportHeight/(2*unit_);
        centerX_=halfWidth>=64?64:std::clamp(centerX_,halfWidth,128-halfWidth);
        centerY_=halfHeight>=48?48:std::clamp(centerY_,halfHeight,96-halfHeight);
        mapX_=viewportCenterX_-centerX_*unit_;mapY_=viewportCenterY_-centerY_*unit_;
    }
    void updateTerrain(ra::Engine& engine) {
        phase_=static_cast<int>(std::fmod(world_.elapsed,300.0f)/75);
        (void)engine;
    }
    isla::World world_;
    std::filesystem::path assetRoot_;
    std::string capturePath_;
    ra::Texture terrain_{},residentsTexture_{},fieldsTexture_{},objectsTexture_{},houseTexture_{},roadTexture_{},natureTexture_{},buildingsTexture_{};
    std::array<ra::Rect,8> natureRegions_{},buildingRegions_{};
    std::size_t selected_{};
    int selectedProp_{-1}, speed_{1}, phase_{-1}, frames_{};
    SDL_Renderer* renderer_{};
    float zoom_{1},centerX_{64},centerY_{48},viewportCenterX_{},viewportCenterY_{},lastMouseX_{},lastMouseY_{};
    bool dragging_{};
    float mapX_{20},mapY_{20},unit_{8},accumulator_{};
    bool paused_{},raining_{},smoke_{};
};
}
int main(int argc,char** argv) {
    try {
        const bool capture=argc>2 && std::string(argv[1])=="--capture";
        const bool smoke=capture || (argc>1 && std::string(argv[1])=="--smoke-test");
        ra::EngineConfig config;config.title="Isla Viva";config.width=1440;config.height=900;
        ra::Engine engine(config);IslaVivaGame game(smoke,std::filesystem::absolute(argv[0]).parent_path()/"assets",capture?argv[2]:"");
        const int result=engine.run(game);
        if(smoke) std::cout << "Native render smoke test finished: " << result << '\n';
        return result;
    } catch(const std::exception& error) { std::cerr << error.what() << '\n';return 1; }
}




