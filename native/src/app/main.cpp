#include <ra/Engine.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "World.h"
#include "Multiplayer.h"
#include "TerrainRenderer.h"
#include "TerrainMaterials.h"
#include "TerrainProjection.h"
#include "SceneTexture.h"
#include "ResidentSprites.h"
#include "ActionPreview.h"
#include "PresentationMotion.h"
#include "FamilyAudio.h"
#include "MysteriousGesture.h"
#include "MysteriousView.h"
#include <chrono>
#include "SessionDialog.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <fstream>
#include <future>
#include <memory>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr float maximumZoom=32.0f;
double realSeconds(){return std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();}
ra::Color color(isla::cnc::Color c) { return {c.r,c.g,c.b,c.a}; }
class IslaVivaGame final : public ra::Game {
public:
    explicit IslaVivaGame(bool smoke, std::filesystem::path assets, std::string capture,std::string profile,std::string join,unsigned short port,bool host,std::string name,bool fresh,unsigned seed): name_(std::move(name)),fresh_(fresh),seed_(seed), profile_(std::move(profile)),joinIp_(std::move(join)),port_(port),hostOnStart_(host),assetRoot_(std::move(assets)), capturePath_(std::move(capture)), raining_(smoke), smoke_(smoke) {}
    void enableActionPreview(int phase){actionPreview_=phase;paused_=true;}
    void enableMysteryPreview(){mysteryPreview_=true;paused_=true;}
    void enableNightPreview(){nightPreview_=true;}
    void previewView(float zoom,float x,float y){zoom_=std::clamp(zoom,1.0f,maximumZoom);centerX_=x;centerY_=y;}
    void onStart(ra::Engine& engine) override {
        engine.setWindowTitle("Isla Viva | Simulacion nativa");
        int count=0; SDL_Window** windows=SDL_GetWindows(&count);
        renderer_=count>0?SDL_GetRenderer(windows[0]):nullptr; SDL_free(windows);
        if(!renderer_) throw std::runtime_error("Cannot access map renderer");


        civicTexture_.load(renderer_,(assetRoot_/"buildings/civic-reference-v5.png").string(),4);
        std::copy_n(civicTexture_.sprites.begin(),4,civicRegions_.begin());
        natureTexture_.load(renderer_,(assetRoot_/"environment/nature-reference-v5.png").string(),8);
        buildingsTexture_.load(renderer_,(assetRoot_/"buildings/buildings-reference-v5.png").string(),8);
        natureRegions_=natureTexture_.sprites;buildingRegions_=buildingsTexture_.sprites;
        residentSprites_.load(renderer_,assetRoot_);
        mysteriousView_.load(renderer_,assetRoot_);
        animalsTexture_=engine.loadTexture((assetRoot_/"characters/residents.png").string());
        if(!animalsTexture_ || !civicTexture_ || !natureTexture_ || !buildingsTexture_) throw std::runtime_error("Faltan recursos del mapa en la carpeta assets");
        bool loaded=!smoke_&&!fresh_&&world_.load(savePath());
        if(!loaded&&!smoke_&&!fresh_){auto old8=std::filesystem::path(savePath()).parent_path()/(profile_=="default"?"world-v8.json":profile_+"-v8.json");loaded=world_.load(old8.string());}
        if(!loaded&&!smoke_&&!fresh_){auto old=std::filesystem::path(savePath()).parent_path()/(profile_=="default"?"world-v7.json":profile_+"-v7.json");loaded=world_.load(old.string());if(!loaded&&profile_=="default")loaded=world_.load((old.parent_path()/"world-v6.json").string());}
        if(!loaded){if(!smoke_&&std::filesystem::exists(savePath()))std::filesystem::copy_file(savePath(),savePath()+".bak",std::filesystem::copy_options::overwrite_existing);world_.generateIsland(seed_?seed_:static_cast<unsigned>(SDL_GetTicksNS()));}
        world_.playerName=name_;engine.setWindowTitle("Isla Viva | "+world_.playerName+" | "+profile_);
        isla::loadTerrainMaterials(terrainPack_,(assetRoot_/"terrain/terrain-materials-v5.png").string());
        message_=loaded?"Partida recuperada; caminos heredados migrados":"Nueva isla aleatoria generada";
        if(hostOnStart_)network_.host(port_);else if(!joinIp_.empty())network_.join(joinIp_,port_);
        world_.updateFishing(0);
        if(nightPreview_){for(int i=0;i<1200;++i){world_.elapsed=isla::World::daySeconds*21/24;world_.tick(.1f);}std::cout<<"Night preview indoors: "<<std::count_if(world_.residents.begin(),world_.residents.end(),[](const isla::Resident& r){return r.alive&&r.insideHouse>=0;})<<" / "<<world_.residents.size()<<"\n";}
        if(smoke_&&actionPreview_>=0)isla::setupActionPreview(world_,actionPreview_);
        if(!smoke_){familyAudio_.load(assetRoot_);familyAudio_.prime(world_);}
        int mysteryDestination=-1;
        if(smoke_&&mysteryPreview_){const auto target=isla::MysteryTarget{isla::MysteryKind::Resident,0};const auto source=isla::MysteriousPower::position(world_,target);for(int dy=-4;dy<=4&&mysteryDestination<0;++dy)for(int dx=-4;dx<=4;++dx){const int x=static_cast<int>(source.first)+dx,y=static_cast<int>(source.second)+dy;if(x>=0&&y>=0&&x<isla::mapWidth&&y<isla::mapHeight&&isla::MysteriousPower::move(world_,target,isla::cellId(x,y),realSeconds())){mysteryDestination=isla::cellId(x,y);previewView(12,x+.5f,y+.5f);break;}}if(mysteryDestination<0)throw std::runtime_error("Mystery preview could not perform move");}
        updateTerrain(engine);
        if(mysteryDestination>=0)mysteriousView_.flash(mysteryDestination,realSeconds());
        SDL_AddEventWatch(textEvents,this);
    }
    void onStop(ra::Engine& engine) override {
        if(terrainWork_.valid())terrainWork_.wait();
        SDL_RemoveEventWatch(textEvents,this);
        generatedTerrain_.reset();civicTexture_.reset();natureTexture_.reset();buildingsTexture_.reset();
        familyAudio_.reset();
        mysteriousView_.reset();
        residentSprites_.reset();
        engine.destroyTexture(animalsTexture_);
        if(!smoke_) world_.save(savePath());
    }
    void onUpdate(ra::Engine& engine,float delta) override {
        if(chatOpen_){
            if(engine.keyPressed(ra::Key::Escape)){chatOpen_=false;setTextInput(false);}
            if(chatSubmit_){chatSubmit_=false;message_=world_.sendChat(chatText_);chatText_.clear();}
            updateNetwork(delta);advanceSimulation(engine,delta);return;
        }
        if (engine.keyPressed(ra::Key::Escape)) { if(mysteriousGesture_.target()){mysteriousGesture_.cancel();return;} if(!tool_.empty()){tool_.clear();relocateSource_=-1;message_="Orden de construccion cancelada";}else engine.quit(); return; }
        if (smoke_ && ++frames_>=(!capturePath_.empty()?4:180)) { engine.quit(); return; }
        if (engine.keyPressed(ra::Key::Space)){paused_=!paused_;motion_.capture(world_);accumulator_=0;}
        if (engine.keyPressed(ra::Key::E)) speed_=speed_==1 ? 4 : speed_==4 ? 12 : 1;
        if (engine.keyPressed(ra::Key::Q)) message_=world_.miracle("rain",0);
        layout(engine);
        const bool overMap=engine.mouseX()>=20 && engine.mouseX()<engine.windowWidth()-352
            && engine.mouseY()>=20 && engine.mouseY()<engine.windowHeight()-20;
        if(overMap && engine.mouseWheelY()!=0) {
            const float worldX=(engine.mouseX()-mapX_)/unit_,worldY=(engine.mouseY()-mapY_)/unit_;
            zoom_=std::clamp(zoom_*std::pow(1.15f,engine.mouseWheelY()),1.0f,maximumZoom);
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
        if(engine.keyPressed(ra::Key::Enter)) {zoom_=1;centerX_=isla::mapWidth*.5f;centerY_=isla::mapHeight*.5f;}
        layout(engine);
        const auto mysteryPoint=groundPoint(engine.mouseX(),engine.mouseY());
        const isla::MysteryPointer mysteryPointer{engine.mousePressed(ra::MouseButton::Left),engine.mouseDown(ra::MouseButton::Left),overMap,engine.mouseX(),engine.mouseY(),mysteryPoint.first,mysteryPoint.second};
        if(mysteriousGesture_.update(world_,mysteryPointer,realSeconds(),tool_.empty()&&!dragging_)){
            pendingMysteryCell_=mysteriousGesture_.destination();motion_.capture(world_);
        }
        if(engine.mousePressed(ra::MouseButton::Left) && !overMap) for(const auto& control:controls_) if(engine.mouseX()>=control.rect.x && engine.mouseX()<control.rect.x+control.rect.w && engine.mouseY()>=control.rect.y && engine.mouseY()<control.rect.y+control.rect.h){activate(engine,control.id);break;}
        if (overMap && engine.mousePressed(ra::MouseButton::Left)) {
            const auto point=groundPoint(engine.mouseX(),engine.mouseY());const float x=point.first,y=point.second;
            if(x>=0&&y>=0&&x<isla::mapWidth&&y<isla::mapHeight) {
                const int cell=isla::cellId(static_cast<int>(x),static_cast<int>(y));
                if(!tool_.empty()) {
                    if(tool_=="grow")message_=world_.growIsland(cell);
                    else if(tool_=="dock"||tool_=="townhall"||tool_=="altar"){
                        auto before=world_.jobs.size();message_=world_.designate(tool_,cell);
                        if(world_.jobs.size()>before){world_.orderUnit(static_cast<int>(selected_),"job",world_.jobs.back().id);tool_.clear();}
                    }else message_=world_.miracle(tool_,static_cast<int>(selected_),cell);
                } else {
                    float nearest=2;selectedProp_=-1;
                    for(std::size_t i=0;i<world_.residents.size();++i){const auto& person=world_.residents[i];if(!person.alive||person.insideHouse>=0||person.away||person.inDungeon||person.inBoat)continue;float distance=std::hypot(x-person.x,y-person.y);if(distance<nearest){nearest=distance;selected_=i;}}
                    if(auto* object=world_.propAt(cell)){selectedProp_=object->uid;if(object->kind=="house"&&object->owner>=0&&object->owner<static_cast<int>(world_.residents.size()))selected_=object->owner;}
                }
            }
        }
        updateNetwork(delta);
        if(!smoke_){autosaveClock_+=delta;if(autosaveClock_>=45){autosaveClock_=0;if(!world_.save(savePath()))message_="No se pudo guardar la partida";}}
        advanceSimulation(engine,delta);
    }
    void onRender(ra::Engine& engine) override {
        layout(engine);
        engine.clear({14,25,31,255});
        const SDL_Rect mapClip{20,20,std::max(1,engine.windowWidth()-372),std::max(1,engine.windowHeight()-40)};
        SDL_SetRenderClipRect(renderer_,&mapClip);
        drawTerrain(engine);
        mysteriousView_.drawFish(world_,mapX_,mapY_,unit_,animationClock_.seconds(),mysteriousGesture_.active()?mysteriousGesture_.target():isla::MysteryTarget{});
        const auto gold=color(isla::cnc::residentColors[0]);
        // Every visible cell and object is drawn from the current mutable world state.
        for(const auto& crop:world_.crops) {
            if(mysteriousGesture_.holding(isla::MysteryKind::Crop,crop.cell))continue;
            const float x=static_cast<float>(crop.cell%isla::mapWidth), y=static_cast<float>(crop.cell/isla::mapWidth);
            const int variant=crop.stage==0?5:crop.stage==3?4:7;
            drawPiece(engine,natureTexture_,natureRegions_[variant],x+.5f,y+1.0f,1.2f);
        }
        drawEntities(engine);
        for(const auto& job:world_.jobs)engine.drawRect({mapX_+(job.cell%isla::mapWidth)*unit_,mapY_+(job.cell/isla::mapWidth-elevationView(job.cell%isla::mapWidth+.5f,job.cell/isla::mapWidth+.5f))*unit_,unit_,unit_},{249,186,78,255});
        if(!tool_.empty()) {
            const auto point=groundPoint(engine.mouseX(),engine.mouseY());const int x=static_cast<int>(std::floor(point.first)),y=static_cast<int>(std::floor(point.second));
            if(x>=0&&y>=0&&x<isla::mapWidth&&y<isla::mapHeight)engine.drawRect({mapX_+x*unit_,mapY_+(y-elevationView(x+.5f,y+.5f))*unit_,unit_*(tool_=="house"?2:1),unit_*(tool_=="house"?2:1)},{255,222,123,255});
        }
        if(world_.night()) engine.fillRect({mapX_,mapY_,isla::mapWidth*unit_,isla::mapHeight*unit_},{12,26,62,90});
        if(world_.alarmSeconds>0) {
            const float x=mapX_+world_.alarmX*unit_, y=mapY_+world_.alarmY*unit_;
            engine.drawLine(x-6,y-6,x+6,y+6,{240,80,64,255});
            engine.drawLine(x-6,y+6,x+6,y-6,{240,80,64,255});
        }
        if(world_.raining) {
            const int count=std::min(300,static_cast<int>(isla::cnc::rainDensity/4));
            for(int i=0;i<count;++i) {
                const float rainSpeed=isla::cnc::rainMinSpeed+(isla::cnc::rainMaxSpeed-isla::cnc::rainMinSpeed)*static_cast<float>(i%17)/16;
                const float x=mapX_+std::fmod(i*17.13f+animationClock_.seconds()*2,isla::mapWidth*unit_);
                const float y=mapY_+std::fmod(i*11.71f+animationClock_.seconds()*rainSpeed*unit_,isla::mapHeight*unit_);
                const float alpha=isla::cnc::rainMinAlpha+(isla::cnc::rainMaxAlpha-isla::cnc::rainMinAlpha)*static_cast<float>(i%7)/6;
                engine.drawLine(x,y,x-2,y+5,{160,197,220,static_cast<unsigned char>(alpha*255)});
            }
        }
        if(mysteriousGesture_.active()){
            const isla::MysteryArt art{natureTexture_,buildingsTexture_,civicTexture_,natureRegions_,buildingRegions_,civicRegions_,residentSprites_,animalsTexture_};
            const auto point=groundPoint(engine.mouseX(),engine.mouseY());
            const bool valid=point.first>=0&&point.second>=0&&point.first<isla::mapWidth&&point.second<isla::mapHeight&&isla::MysteriousPower::canDrop(world_,mysteriousGesture_.target(),isla::cellId(static_cast<int>(point.first),static_cast<int>(point.second)));
            mysteriousView_.drawHeld(engine,world_,mysteriousGesture_.target(),engine.mouseX(),engine.mouseY(),unit_,animationClock_.seconds(),art,valid);
        }
        mysteriousView_.drawEffect(renderer_,world_,{20,20,static_cast<float>(mapClip.w),static_cast<float>(mapClip.h)},mapX_,mapY_,unit_,realSeconds());
        SDL_SetRenderClipRect(renderer_,nullptr);
        const float panel=std::max(12.0f,static_cast<float>(engine.windowWidth())-324);
        engine.fillRect({panel,16,308,static_cast<float>(engine.windowHeight())-32},{28,42,48,255});
        float y=34;
        auto text=[&](const std::string& value,ra::Color c) {engine.drawText(panel+14,y,value,c);y+=20;};
        const ra::Color white{214,225,212,255};
        text("ISLA DE "+world_.playerName,gold);
        text("DIA "+std::to_string(static_cast<int>(world_.elapsed/300)+1)+"  "+std::to_string(speed_)+"X",white);
        constexpr const char* times[]{"MANANA","TARDE","ATARDECER","NOCHE"};
        float hour=std::fmod(world_.elapsed/isla::World::daySeconds*24,24.0f);text(world_.night()?"NOCHE":hour<12?"MANANA":hour<18?"TARDE":"ATARDECER",white);
        text(paused_?"PAUSA / ESPACIO":"ESPACIO: PAUSA",white);
        text("E: VELOCIDAD  Q: LLUVIA",white);
        text("RUEDA: ZOOM "+std::to_string(static_cast<int>(std::round(zoom_*100)))+"%",white);
        text("CENTRAL / WASD: MOVER",white);
        text("ENTER: VER ISLA ENTERA",white);
        text("CLIC: INSPECCIONAR",white);
        text("LOS HABITANTES DECIDEN SUS TAREAS",white);
        y+=10;
        const auto& r=world_.residents[selected_];
        text(r.name+" / "+isla::World::lifeStageName(r),color(isla::cnc::residentColors[selected_%isla::cnc::residentColors.size()]));
        std::string activity=r.insideHouse>=0?"DESCANSANDO DENTRO DE CASA":r.away?"DE VISITA EN OTRA ISLA":r.visitor?"VISITANTE DE OTRA ISLA":r.ageDays<isla::World::adultAgeDays?"MENOR AL CUIDADO DE LA ALDEA":isla::World::taskName(r.task);
        for(const auto& boat:world_.ships)if(boat.passenger==static_cast<int>(selected_)){
            if(boat.state=="visit_boarding")activity="CAMINANDO AL MUELLE";
            else if(boat.state=="visit_outbound")activity="NAVEGANDO A OTRA ISLA";
            else if(boat.state=="visit_waiting")activity="DE VISITA EN OTRA ISLA";
            else if(boat.state=="visit_returning")activity="REGRESANDO EN BARCO";
        }
        text(activity,white);
        text("COMIDA "+std::to_string(static_cast<int>(r.hunger))+"  AGUA "+std::to_string(static_cast<int>(r.thirst)),white);
        text("ENERGIA "+std::to_string(static_cast<int>(r.energy))+" EDAD "+std::to_string(static_cast<int>(r.ageDays/360))+" ANOS",white);
        text("SOCIAL "+std::to_string(static_cast<int>(r.social))+"  DIVERSION "+std::to_string(static_cast<int>(r.fun)),white);
        text("HIGIENE "+std::to_string(static_cast<int>(r.hygiene)),white);
        text("SALUD "+std::to_string(static_cast<int>(r.health))+" / "+std::to_string(static_cast<int>(isla::cnc::maxHealth)),white);
        if(r.pregnancyDue>=0)text("EMBARAZO: "+std::to_string(static_cast<int>(std::ceil((r.pregnancyDue-world_.elapsed)/isla::World::daySeconds)))+" DIAS",gold);
        text("GRANO "+std::to_string(world_.grain),gold);
        y+=10;
        text("LA ALDEA",gold);
        text(std::to_string(std::count_if(world_.residents.begin(),world_.residents.end(),[](const isla::Resident& p){return p.alive;}))+" HABITANTES VIVOS",white);
        text("MADERA "+std::to_string(world_.communityWood)+" MONEDAS "+std::to_string(world_.treasury),white);
        text("PESCADO "+std::to_string(r.fish),white);
        text(world_.chosenBirthAt>=0?"ELEGIDO EN "+std::to_string(static_cast<int>(std::ceil((world_.chosenBirthAt-world_.elapsed)/isla::World::daySeconds)))+" DIAS":network_.status().substr(0,34),white);
        text(world_.raining ? "ESTA LLOVIENDO" : "CIELO DESPEJADO",white);

        controls_.clear();float buttonY=y+8;int column=0;
        auto button=[&](const std::string& id,const std::string& title){
            const ra::Rect rectangle{panel+12+column*144.0f,buttonY,138,22};
            engine.fillRect(rectangle,tool_==id?ra::Color{86,77,44,255}:ra::Color{46,66,73,255});
            engine.drawText(rectangle.x+5,rectangle.y+8,title,white);controls_.push_back({rectangle,id});
            if(++column==2){column=0;buttonY+=25;}
        };
        button("select","INSPECCIONAR");button("rain","ENVIAR LLUVIA");
        button("gift","DEJAR REGALO");button("heal","MILAGRO: SANAR");
        button("sustain","INSP. SUSTENTO");button("build","INSP. EDIFICAR");
        button("diplomacy","INSP. DIPLOMACIA");button("raid","INSP. SAQUEO");
        button("chosen","ANUNCIAR ELEGIDO");button("host",network_.hosting()?"CERRAR MI ISLA":"ABRIR MI ISLA");
                button("join","CONECTAR ISLA");button("chat","CHAT");
        button("dock","ENCARGO MUELLE");button("townhall","TOWNHALL");
        button("altar","ENCARGO ALTAR");button("grow","AMPLIAR ISLA");
        button("save","GUARDAR");button("load","CARGAR");
        if(!tool_.empty())engine.drawText(panel+12,buttonY+8,"HERRAMIENTA: "+tool_,gold);
        const float statusY=static_cast<float>(engine.windowHeight())-62;
        engine.fillRect({panel+6,statusY-4,296,48},{20,33,38,255});
        const std::string status=message_.empty()?"Influye en los habitantes o contempla su autonomia":message_;
        for(std::size_t i=0;i<status.size()&&i<102;i+=34)engine.drawText(panel+12,statusY+static_cast<float>(i/34)*12,status.substr(i,34),gold);
        if(chatOpen_||!world_.chatLog.empty()){
            float bottom=static_cast<float>(engine.windowHeight())-24;engine.fillRect({28,bottom-150,std::max(200.0f,panel-60),148},{20,34,42,235});
            engine.drawText(38,bottom-140,"CHAT ENTRE ISLAS - "+world_.peerName,white);
            std::vector<std::string> lines;std::size_t columns=static_cast<std::size_t>(std::max(20.0f,(panel-90)/6));
            for(const auto& entry:world_.chatLog)for(std::size_t at=0;at<entry.size();at+=columns)lines.push_back(entry.substr(at,columns));
            int row=0;std::size_t first=lines.size()>5?lines.size()-5:0;for(std::size_t i=first;i<lines.size();++i)engine.drawText(38,bottom-118+row++*18,lines[i],white);
            engine.drawText(38,bottom-24,chatOpen_?"> "+chatText_.substr(chatText_.size()>110?chatText_.size()-110:0):"Boton CHAT para escribir. Enter envia; Esc cierra.",gold);
        }
        if(!capturePath_.empty() && frames_==2) {
            int count=0;
            SDL_Window** windows=SDL_GetWindows(&count);
            SDL_Surface* surface=count>0?SDL_RenderReadPixels(SDL_GetRenderer(windows[0]),nullptr):nullptr;
            SDL_free(windows);
            if(!surface) throw std::runtime_error("Cannot capture map preview");
            const bool saved=std::filesystem::path(capturePath_).extension()==".png"
                ?IMG_SavePNG(surface,capturePath_.c_str()):SDL_SaveBMP(surface,capturePath_.c_str());
            SDL_DestroySurface(surface);
            if(!saved) throw std::runtime_error("Cannot save map preview");
        }
    }
private:
    struct Control { ra::Rect rect; std::string id; };
    std::vector<Control> controls_;
    std::string name_;bool fresh_{};unsigned seed_{};isla::TerrainRenderer terrainPack_;isla::SceneTexture generatedTerrain_,civicTexture_,natureTexture_,buildingsTexture_;std::array<ra::Rect,4> civicRegions_{};int drawnRevision_{-1};
    bool chatOpen_{},chatSubmit_{};std::string chatText_;
    isla::PeerLink network_;std::string profile_,joinIp_;unsigned short port_{32145};bool hostOnStart_{};float networkClock_{};
    std::string tool_,message_;int relocateSource_{-1};float autosaveClock_{};
    std::string savePath() const {
        const char* local=std::getenv("LOCALAPPDATA");
        return ((local?std::filesystem::path(local):assetRoot_.parent_path())/"IslaVivaNative"/(profile_=="default"?"world-v9.json":profile_+"-v9.json")).string();
    }
    void activate(ra::Engine& engine,const std::string& id) {
        if(id=="select"){tool_.clear();relocateSource_=-1;message_="Modo inspeccion";}
        else if(id=="host"){if(network_.hosting()){network_.stop();message_="Tu isla esta cerrada a nuevas visitas";}else message_=network_.host(port_)?"Tu direccion: "+isla::localIPv4()+":"+std::to_string(port_):network_.status();}
        else if(id=="join"){if(chooseConnection(engine,joinIp_,port_))message_=network_.join(joinIp_,port_)?"Conectando con la otra isla":network_.status();}
        else if(id=="chat"){chatOpen_=true;setTextInput(true);}
        else if(id=="gift"||id=="dock"||id=="townhall"||id=="altar"||id=="grow"){tool_=id;message_="Selecciona el lugar en el mapa";}
        else if(id=="save")message_=world_.save(savePath())?"Partida guardada":"Error al guardar";
        else if(id=="load"){if(network_.connected()){message_="Cierra la conexion antes de cargar otra partida";return;}message_=world_.load(savePath())?"Partida cargada":"No hay guardado compatible";selected_=std::min(selected_,world_.residents.size()-1);selectedProp_=-1;}
        else message_=world_.miracle(id,static_cast<int>(selected_));
    }
    void updateNetwork(float delta){
        network_.poll();
        if(!network_.connected()){world_.peerId.clear();networkClock_=1;return;}
        for(const auto& raw:network_.receive())try{
            auto m=nlohmann::json::parse(raw);
            if(m.value("type","")=="hello"){
                auto id=m.at("from").get<std::string>();
                if(m.at("protocol")==1&&id.size()==32&&id!=world_.worldId&&std::all_of(id.begin(),id.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');})){world_.peerId=id;world_.peerName=m.value("name",std::string("Otra isla"));}
                else message_="La otra isla usa una identidad incompatible";
            }else if(!world_.peerId.empty()){
                message_=world_.receivePeer(raw,world_.peerId);
                // Durable state precedes acknowledgements or resource transfers.
                if(!smoke_&&!world_.save(savePath()))throw std::runtime_error("No se pudo guardar el estado multijugador");
            }
        }catch(const std::exception&){message_="Mensaje de red rechazado";}
        networkClock_+=delta;if(networkClock_<1)return;networkClock_=0;
        if(!smoke_&&!world_.save(savePath())){message_="Red detenida: no se puede guardar";return;}
        network_.send(nlohmann::json{{"protocol",1},{"type","hello"},{"from",world_.worldId},{"name",world_.playerName}}.dump());
        std::vector<std::string> acknowledged;
        for(const auto& pending:world_.outbox){auto m=nlohmann::json::parse(pending.second);if(m.at("to")==world_.peerId){network_.send(pending.second);if(m.at("type")=="confirmed"||m.at("type")=="chat_ack")acknowledged.push_back(pending.first);}}
        for(const auto& id:acknowledged)world_.outbox.erase(id);
    }
    static bool SDLCALL textEvents(void* context,SDL_Event* event){auto& self=*static_cast<IslaVivaGame*>(context);if(!self.chatOpen_)return true;
        if(event->type==SDL_EVENT_TEXT_INPUT){std::string text=event->text.text;if(self.chatText_.size()+text.size()<=256)self.chatText_+=text;}
        if(event->type==SDL_EVENT_KEY_DOWN){if(event->key.key==SDLK_BACKSPACE&&!self.chatText_.empty()){do{unsigned char c=static_cast<unsigned char>(self.chatText_.back());self.chatText_.pop_back();if((c&0xc0)!=0x80)break;}while(!self.chatText_.empty());}if(event->key.key==SDLK_RETURN)self.chatSubmit_=true;}
        return true;
    }
    void setTextInput(bool enable){int count=0;auto windows=SDL_GetWindows(&count);if(count){if(enable)SDL_StartTextInput(windows[0]);else SDL_StopTextInput(windows[0]);}SDL_free(windows);}
    void advanceSimulation(ra::Engine& engine,float delta){
        animationClock_.advance(delta,paused_);
        if(!paused_){accumulator_+=std::clamp(delta,0.0f,.25f)*static_cast<float>(speed_);while(accumulator_>=.1f){motion_.capture(world_);world_.tick(.1f);if(!smoke_)familyAudio_.update(world_,false);accumulator_-=.1f;}}
        if(!smoke_)familyAudio_.update(world_,paused_);
        int phase=static_cast<int>(std::fmod(world_.elapsed,300.0f)/75);if(phase!=phase_||drawnRevision_!=world_.terrainRevision||terrainWork_.valid())updateTerrain(engine);
        if(pendingMysteryCell_>=0){mysteriousView_.flash(pendingMysteryCell_,realSeconds());pendingMysteryCell_=-1;}
    }
    float elevationView(float x,float y) const{return isla::TerrainProjection::elevation(world_,x,y);}
    std::pair<float,float> groundPoint(float mouseX,float mouseY) const{
        return isla::TerrainProjection::groundPoint(world_,(mouseX-mapX_)/unit_,(mouseY-mapY_)/unit_);
    }
    void drawPiece(ra::Engine& engine,const isla::SceneTexture& texture,ra::Rect source,float x,float footY,float width) {
        const float height=width*source.h/std::max(1.0f,source.w);
        const float left=mapX_+(x-width*.5f)*unit_,bottom=mapY_+(footY-elevationView(x,footY))*unit_;
        if(left+width*unit_<20||left>engine.windowWidth()-352||bottom<20||bottom-height*unit_>engine.windowHeight()-20)return;
        if(!isla::TerrainProjection::visible(world_,x,footY))return;
        (void)engine;texture.draw(source,{mapX_+(x-width*.5f)*unit_,mapY_+(footY-elevationView(x,footY)-height)*unit_,width*unit_,height*unit_});
    }
    void drawEntities(ra::Engine& engine) {
        residentSprites_.prepare(world_);
        struct Item {float bottom;int kind;std::size_t index;};std::vector<Item> draw;
        for(std::size_t i=0;i<world_.props.size();++i)draw.push_back({world_.props[i].y+world_.props[i].halfDepth,0,i});
        for(std::size_t i=0;i<world_.animals.size();++i)if(world_.animals[i].alive)draw.push_back({world_.animals[i].y+.2f,1,i});
        for(std::size_t i=0;i<world_.residents.size();++i){const auto& r=world_.residents[i];if(r.alive&&r.insideHouse<0&&!r.inDungeon&&!r.away&&!r.inBoat)draw.push_back({r.y+.2f,2,i});}
        for(std::size_t i=0;i<world_.camps.size();++i)draw.push_back({static_cast<float>(world_.camps[i].cell/isla::mapWidth)+1,3,i});
        for(std::size_t i=0;i<world_.gifts.size();++i)draw.push_back({static_cast<float>(world_.gifts[i].cell/isla::mapWidth)+1,4,i});
        std::sort(draw.begin(),draw.end(),[](const Item& a,const Item& b){return a.bottom<b.bottom;});
        for(const auto& ship:world_.ships)if(ship.seaHome>=0)drawPiece(engine,buildingsTexture_,buildingRegions_[4],ship.cell%isla::mapWidth+.5f,ship.cell/isla::mapWidth+1.2f,2.6f);
        for(const auto& item:draw){
            if((item.kind==0&&mysteriousGesture_.holding(isla::MysteryKind::Prop,world_.props[item.index].uid))||(item.kind==1&&mysteriousGesture_.holding(isla::MysteryKind::Animal,static_cast<int>(item.index)))||(item.kind==2&&mysteriousGesture_.holding(isla::MysteryKind::Resident,static_cast<int>(item.index)))||(item.kind==3&&mysteriousGesture_.holding(isla::MysteryKind::Camp,world_.camps[item.index].owner))||(item.kind==4&&mysteriousGesture_.holding(isla::MysteryKind::Gift,world_.gifts[item.index].cell)))continue;
            if(item.kind==0){const auto& p=world_.props[item.index];if(p.tree)drawPiece(engine,natureTexture_,natureRegions_[p.health<40?3:((static_cast<unsigned>(p.uid)*1103515245u)>>16)%4==0?1:0],p.x+static_cast<float>((p.uid*17)%7-3)*.045f,p.y+.5f,2.5f+(p.uid%7)*.14f);else if(p.kind=="crystal")drawPiece(engine,civicTexture_,civicRegions_[3],p.x,p.y+.5f,3.0f);else if(p.kind=="townhall"||p.kind=="altar"||p.kind=="dock")drawPiece(engine,civicTexture_,civicRegions_[p.kind=="townhall"?0:p.kind=="altar"?1:2],p.x,p.y+1,p.kind=="townhall"?3.4f:2.6f);else drawPiece(engine,buildingsTexture_,buildingRegions_[p.uid%4],p.x,p.y+1,2.6f);}
            else if(item.kind==1){const auto& a=world_.animals[item.index];engine.drawTextureRegion(animalsTexture_,{static_cast<float>(a.kind*8),32,8,8},{mapX_+(a.x-.5f)*unit_,mapY_+(a.y-.5f-elevationView(a.x,a.y))*unit_,unit_,unit_});}
            else if(item.kind==2){
                const auto& r=world_.residents[item.index];
                if(!isla::TerrainProjection::visible(world_,r.x,r.y))continue;
                const auto position=motion_.resident(world_,item.index,paused_?1:accumulator_/.1f);
                const float footX=mapX_+position.x*unit_,footY=mapY_+(position.y-elevationView(position.x,position.y))*unit_;
                residentSprites_.draw(r,world_.elapsed,footX,footY,unit_,static_cast<int>(item.index),animationClock_.seconds());
                if(item.index==selected_)engine.drawRect({footX-unit_*.35f,footY-unit_,unit_*.7f,unit_},{249,206,122,255});
            }
            else if(item.kind==4){int k=world_.gifts[item.index].cell;engine.fillRect({mapX_+(k%isla::mapWidth+.2f)*unit_,mapY_+(k/isla::mapWidth+.2f)*unit_,unit_*.6f,unit_*.6f},{247,207,83,255});}
            else{const auto& camp=world_.camps[item.index];drawPiece(engine,buildingsTexture_,buildingRegions_[0],camp.cell%isla::mapWidth+.5f,camp.cell/isla::mapWidth+1.0f,1.6f);}
        }
        for(const auto& ship:world_.ships){
            if(ship.seaHome<0)continue;

            if(ship.state=="lost")continue;
            const bool aboard=ship.passenger>=0&&ship.passenger<static_cast<int>(world_.residents.size())&&world_.residents[ship.passenger].inBoat;
            if(aboard&&!world_.residents[ship.passenger].away){
                const auto position=motion_.boat(ship,static_cast<std::size_t>(&ship-world_.ships.data()),paused_?1:accumulator_/.1f);
                residentSprites_.drawBoat(ship,world_.residents[ship.passenger],animationClock_.seconds(),mapX_+position.x*unit_,mapY_+(position.y-elevationView(position.x,position.y))*unit_,unit_);
            }else if(!aboard){
                float x=ship.x,y=ship.y;
                if((ship.state=="boarding"||ship.state=="visit_boarding")&&ship.fishingClock>0){
                    const float progress=std::clamp(ship.fishingClock/3.0f,0.0f,1.0f);
                    x+=(ship.cell%isla::mapWidth+.5f-ship.x)*(1-progress)*.7f;
                    y+=(ship.cell/isla::mapWidth+.5f-ship.y)*(1-progress)*.7f;
                }
                residentSprites_.drawEmptyBoat(ship,animationClock_.seconds(),mapX_+x*unit_,mapY_+(y-elevationView(x,y))*unit_,unit_);
                if((ship.state=="boarding"||ship.state=="visit_boarding")&&ship.fishingClock>0&&ship.passenger>=0&&ship.passenger<static_cast<int>(world_.residents.size())){
                    const auto& worker=world_.residents[ship.passenger];
                    residentSprites_.draw(worker,world_.elapsed,mapX_+worker.x*unit_,mapY_+(worker.y-elevationView(worker.x,worker.y))*unit_,unit_,ship.passenger,animationClock_.seconds());
                }
            }
        }
        residentSprites_.drawFamilyEffects(world_,mapX_,mapY_,unit_,animationClock_.seconds());
    }
    void drawTerrain(ra::Engine& engine) {
        generatedTerrain_.draw({mapX_,mapY_,isla::mapWidth*unit_,isla::mapHeight*unit_});
        auto road=[&](int x,int y){auto t=world_.terrainAt(x,y);return t==isla::Terrain::Road||t==isla::Terrain::Bridge;};
        const int left=std::max(0,static_cast<int>(std::floor((20-mapX_)/unit_))),right=std::min(isla::mapWidth,static_cast<int>(std::ceil((engine.windowWidth()-352-mapX_)/unit_)));
        const int top=std::max(0,static_cast<int>(std::floor((20-mapY_)/unit_))),bottom=std::min(isla::mapHeight,static_cast<int>(std::ceil((engine.windowHeight()-20-mapY_)/unit_)));
        const int radius=std::max(1,static_cast<int>(unit_*.16f));
        for(int y=top;y<bottom;++y)for(int x=left;x<right;++x){auto type=world_.terrainAt(x,y);if(type==isla::Terrain::Bridge){drawPiece(engine,buildingsTexture_,buildingRegions_[7],x+.5f,y+1.0f,1.8f);continue;}if(type!=isla::Terrain::Road)continue;float cx=mapX_+(x+.5f)*unit_,cy=mapY_+(y+.5f-elevationView(x+.5f,y+.5f))*unit_;for(int yy=-radius;yy<=radius;++yy){int half=static_cast<int>(std::sqrt(static_cast<float>(radius*radius-yy*yy)));engine.drawLine(cx-half,cy+yy,cx+half,cy+yy,{194,169,116,255});}for(auto step:std::array<std::array<int,2>,2>{{{1,0},{0,1}}})if(road(x+step[0],y+step[1]))for(int offset=-radius;offset<=radius;++offset)engine.drawLine(cx+(step[1]?offset:0),cy+(step[0]?offset:0),cx+step[0]*unit_+(step[1]?offset:0),cy+step[1]*unit_+(step[0]?offset:0),{194,169,116,255});}
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
        centerX_=halfWidth>=isla::mapWidth*.5f?isla::mapWidth*.5f:std::clamp(centerX_,halfWidth,isla::mapWidth-halfWidth);
        centerY_=halfHeight>=isla::mapHeight*.5f?isla::mapHeight*.5f:std::clamp(centerY_,halfHeight,isla::mapHeight-halfHeight);
        mapX_=viewportCenterX_-centerX_*unit_;mapY_=viewportCenterY_-centerY_*unit_;
    }
    struct TerrainFrame {int revision;std::vector<unsigned char> pixels;};
    std::future<TerrainFrame> terrainWork_;
    void updateTerrain(ra::Engine& engine) {
        if(!generatedTerrain_) {
            auto pixels=terrainPack_.render(world_);
            generatedTerrain_.upload(renderer_,pixels.data(),isla::TerrainRenderer::width,isla::TerrainRenderer::height);
            drawnRevision_=world_.terrainRevision;
        } else {
            if(terrainWork_.valid()&&terrainWork_.wait_for(std::chrono::seconds(0))==std::future_status::ready) {
                auto frame=terrainWork_.get();
                generatedTerrain_.upload(renderer_,frame.pixels.data(),isla::TerrainRenderer::width,isla::TerrainRenderer::height);
                drawnRevision_=frame.revision;
            }
            if(!terrainWork_.valid()&&drawnRevision_!=world_.terrainRevision) {
                const auto snapshot=std::make_shared<isla::World>(world_);
                const int revision=world_.terrainRevision;
                terrainWork_=std::async(std::launch::async,[this,snapshot,revision] {
                    return TerrainFrame{revision,terrainPack_.render(*snapshot)};
                });
            }
        }
        phase_=static_cast<int>(std::fmod(world_.elapsed,300.0f)/75);
        (void)engine;
    }
    isla::World world_;
    std::filesystem::path assetRoot_;
    std::string capturePath_;
    bool mysteryPreview_{};int pendingMysteryCell_{-1};
    isla::MysteriousGesture mysteriousGesture_;
    isla::MysteriousView mysteriousView_;
    isla::FamilyAudio familyAudio_;
    isla::PresentationClock animationClock_;
    isla::PresentationMotion motion_;
    isla::ResidentSprites residentSprites_;
    ra::Texture animalsTexture_{};
    std::array<ra::Rect,8> natureRegions_{},buildingRegions_{};
    std::size_t selected_{};
    int actionPreview_{-1};
    int selectedProp_{-1}, speed_{1}, phase_{-1}, frames_{};
    SDL_Renderer* renderer_{};
    float zoom_{1},centerX_{isla::mapWidth*.5f},centerY_{isla::mapHeight*.5f},viewportCenterX_{},viewportCenterY_{},lastMouseX_{},lastMouseY_{};
    bool dragging_{};
    float mapX_{20},mapY_{20},unit_{8},accumulator_{};
    bool paused_{},raining_{},smoke_{},nightPreview_{};
};
}
int main(int argc,char** argv) {
    try {
        const bool capture=argc>2 && std::string(argv[1])=="--capture";
        const bool smoke=capture || (argc>1 && std::string(argv[1])=="--smoke-test");
        std::string profile="default",join,name="Deidad";bool host=false,fresh=false,noDialog=false,nightPreview=false,detailPreview=false;unsigned seed=0;unsigned short port=32145;
        for(int i=1;i<argc;++i){std::string option=argv[i];if(option=="--night-preview")nightPreview=true;else if(option=="--detail-preview"||option=="--resident-preview")detailPreview=true;else if(option=="--name"&&i+1<argc)name=argv[++i];else if(option=="--new-island")fresh=true;else if(option=="--no-dialog")noDialog=true;else if(option=="--seed"&&i+1<argc)seed=static_cast<unsigned>(std::stoul(argv[++i]));else if(option=="--host")host=true;else if(option=="--join"&&i+1<argc)join=argv[++i];else if(option=="--profile"&&i+1<argc)profile=argv[++i];else if(option=="--port"&&i+1<argc){int value=std::stoi(argv[++i]);if(value<1024||value>65535)throw std::runtime_error("Puerto invalido");port=static_cast<unsigned short>(value);}}
        ra::EngineConfig config;config.title="Isla Viva";config.width=1440;config.height=900;
        ra::Engine engine(config);
        if(!smoke&&!noDialog){SessionOptions options{name,profile,join,port,host,fresh,seed};if(!chooseSession(engine,options,argc>2&&std::string(argv[1])=="--capture-menu"?argv[2]:""))return 0;name=options.name;profile=options.profile;join=options.ip;port=options.port;host=options.host;fresh=options.fresh;seed=options.seed;}
        if(profile=="default"&&(host||!join.empty()))profile=host?"island-host":"island-guest";
        if(profile.empty()||profile.size()>32||!std::all_of(profile.begin(),profile.end(),[](char c){return std::isalnum(static_cast<unsigned char>(c))||c=='-'||c=='_';}))throw std::runtime_error("Nombre de perfil invalido");
        IslaVivaGame game(smoke,std::filesystem::absolute(argv[0]).parent_path()/"assets",capture?argv[2]:"",profile,join,port,host,name,fresh,seed);
        if(smoke&&nightPreview)game.enableNightPreview();
        for(int i=1;smoke&&i<argc;++i)if(std::string(argv[i])=="--mystery-preview")game.enableMysteryPreview();
        for(int i=1;smoke&&i+1<argc;++i)if(std::string(argv[i])=="--actions-preview"){game.enableActionPreview(std::clamp(std::stoi(argv[i+1]),0,2));game.previewView(8,110,111);}
        if(smoke&&detailPreview){bool close=false;for(int i=1;i<argc;++i)if(std::string(argv[i])=="--resident-preview")close=true;game.previewView(close?8.0f:3.0f,112,111);}
        for(int i=1;smoke&&i+1<argc;++i)if(std::string(argv[i])=="--zoom-preview")game.previewView(std::stof(argv[i+1]),112,111);
        const int result=engine.run(game);
        if(smoke) std::cout << "Native render smoke test finished: " << result << '\n';
        return result;
    } catch(const std::exception& error) { std::cerr << error.what() << '\n';return 1; }
}


