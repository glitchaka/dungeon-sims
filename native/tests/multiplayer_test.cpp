#include "World.h"
#include "Multiplayer.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>
using nlohmann::json;
void check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
void clean(isla::World& w){
    w.ships.clear();w.props.clear();w.crops.clear();w.animals.clear();w.jobs.clear();w.gifts.clear();w.visits.clear();w.outbox.clear();w.forest.fill(false);w.pasture.fill(false);w.protectedLand.fill(false);w.fallow.fill(false);
    w.terrain.fill(isla::Terrain::Ground);w.terrain[isla::cellId(8,8)]=isla::Terrain::FreshWater;w.market=isla::cellId(20,20);w.granary=isla::cellId(21,20);w.grain=100;w.treasury=100;w.elapsed=0;
    w.ecologyClock=w.townClock=w.populationClock=-100000;
    for(std::size_t i=0;i<w.residents.size();++i){auto& r=w.residents[i];r.x=10.5f+static_cast<float>(i);r.y=10.5f;r.home=r.goal=isla::cellId(static_cast<int>(r.x),10);r.health=isla::cnc::maxHealth;r.hunger=r.thirst=r.energy=90;r.alive=true;r.away=r.visitor=r.inBoat=r.inDungeon=false;r.path.clear();r.next=0;r.task=isla::Task::Idle;r.hasOrder=false;r.decisionTimer=100000;r.money=12;r.grain=r.wood=r.fish=0;r.inspiration.clear();r.inspirationUntil=0;}
    for(int y=0;y<isla::mapHeight;++y)for(int x=0;x<7;++x)w.terrain[isla::cellId(x,y)]=isla::Terrain::Sea;
    isla::Ship boat;boat.cell=isla::cellId(7,10);boat.seaHome=isla::cellId(6,10);boat.x=6.5f;boat.y=10.5f;w.ships.push_back(boat);
    w.rebuildNavigation();
}
void depart(isla::World& w,int who,const std::string& kind){
    w.sendVisit(who,kind);check(!w.visits.empty()&&w.visits.back().state=="boarding"&&w.outbox.empty()&&!w.residents[who].away,"Visit transferred before boarding and actual sailing");
    auto resumed=std::make_unique<isla::World>();check(w.save("naval-boarding-test.json")&&resumed->load("naval-boarding-test.json")&&resumed->ships[0].state=="visit_boarding"&&resumed->outbox.empty(),"Boarding voyage did not persist without transferring visitor early");
    for(int i=0;i<4000&&resumed->outbox.empty();++i){resumed->elapsed=100;resumed->tick(.1f);}check(!resumed->outbox.empty()&&resumed->ships[0].state=="visit_waiting","Restored boarding voyage did not resume sailing");
    auto count=w.visits.size();w.sendVisit(who,kind);check(w.visits.size()==count,"Boarding resident assigned to multiple voyages");
    for(int i=0;i<4000&&w.outbox.empty();++i){w.elapsed=100;w.tick(.1f);}
    check(w.residents[who].away&&w.residents[who].inBoat&&!w.outbox.empty()&&w.ships[0].state=="visit_waiting","Visitor never sailed to high sea");
}
void returnHome(isla::World& w,int who){for(int i=0;i<4000&&w.residents[who].away;++i){w.elapsed=100;w.tick(.1f);}check(!w.residents[who].away&&!w.residents[who].inBoat&&w.ships[0].state=="docked","Visitor did not physically sail home and disembark");}
int main(){try{
    auto a=std::make_unique<isla::World>(),b=std::make_unique<isla::World>();clean(*a);clean(*b);check(a->worldId!=b->worldId,"Independent islands share an identity");
    a->miracle("chosen",0);float birth=a->chosenBirthAt;check(birth==9*30*300,"Chosen gestation is not nine game months");
    a->elapsed=birth-.125f;a->updateDivinity(0);check(a->residents.size()==6,"Chosen was born early");
    check(a->save("divine-test.json"),"Cannot persist pregnancy");auto restored=std::make_unique<isla::World>();check(restored->load("divine-test.json")&&restored->chosenBirthAt==birth,"Pregnancy deadline was not preserved");
    restored->elapsed=birth;restored->updateDivinity(0);check(restored->residents.size()==7&&restored->residents.back().chosen&&restored->residents.back().ageDays==0,"Chosen did not arrive as a newborn at deadline");
    restored->updateDivinity(0);check(restored->residents.size()==7,"Chosen birth duplicated");
    restored->residents.back().thirst=30;restored->residents.back().homeWater=0;restored->updateDivinity(0);check(restored->residents.back().thirst==90,"An infant cannot receive water from an adult caregiver");
    a->miracle("rain",0);check(a->raining,"God cannot send rain");a->crops.push_back({isla::cellId(40,40),1,0,-1,0,0});a->updateDivinity(1);check(a->crops.back().growth>0,"Divine rain has no agricultural effect");
    int gift=isla::cellId(10,11);int grain=a->grain,wood=a->communityWood;a->miracle("gift",0,gift);check(a->gifts.size()==1&&!a->walkable(10,11),"Gift is not a collidable world object");a->updateDivinity(0);check(a->gifts.empty()&&a->grain==grain+12&&a->communityWood==wood+8&&a->walkable(10,11),"Resident did not collect a real gift");
    a->residents[0].thirst=1;a->miracle("build",0);a->tick(.1f);check(a->residents[0].task==isla::Task::Drink,"Divine inspiration overrode an urgent survival need");
    clean(*a);a->residents[0].money=0;a->forest[isla::cellId(12,12)]=true;
    a->props.push_back({12.5f,12.5f,.32f,.32f,3,true,"Tree",200,-1,60});a->rebuildNavigation();
    // A working-day NPC must autonomously cut an actual solid tree without a player order.
    a->elapsed=120;a->grain=200;auto& worker=a->residents[0];worker.decisionTimer=0;worker.thirst=worker.hunger=worker.energy=worker.social=worker.fun=worker.hygiene=99;
    bool gathered=false;for(int i=0;i<2000&&!gathered;++i){a->tick(.1f);gathered=worker.wood>=4;}
    check(gathered&&!a->propAt(isla::cellId(12,12)),"NPC forestry still requires the removed player buttons");
    clean(*a);a->peerId=b->worldId;b->peerId=a->worldId;
    isla::PeerLink host,client;unsigned short port=39210;while(port<39240&&!host.host(port))++port;check(port<39240,"Cannot bind TCP test port");check(client.join("127.0.0.1",port),"Cannot connect second TCP player");
    for(int i=0;i<1000&&(!host.connected()||!client.connected());++i){host.poll();client.poll();std::this_thread::sleep_for(std::chrono::milliseconds(1));}
    check(host.connected()&&client.connected(),"TCP peer handshake never connected");
    auto deliver=[&](isla::PeerLink& source,isla::PeerLink& target,isla::World& receiver,const std::string& sender,const std::string& payload){
        source.send(payload);std::vector<std::string> messages;
        for(int i=0;i<1000&&messages.empty();++i){source.poll();target.poll();messages=target.receive();if(messages.empty())std::this_thread::sleep_for(std::chrono::milliseconds(1));}
        check(!messages.empty(),"Actual TCP transport lost a mission message");for(const auto& m:messages)receiver.receivePeer(m,sender);
    };
    a->playerName="Manuel";b->playerName="Otra deidad";a->sendChat("Hola desde mi isla");check(a->outbox.size()==1,"Chat was not queued");auto chatId=a->outbox.begin()->first;auto text=a->outbox.begin()->second;
    deliver(client,host,*b,a->worldId,text);deliver(client,host,*b,a->worldId,text);check(b->chatLog.size()==1&&b->chatLog[0]=="Manuel: Hola desde mi isla","Real TCP chat lost player name or duplicated text");
    deliver(host,client,*a,b->worldId,b->outbox.at("ack:"+chatId));check(a->outbox.empty(),"Chat acknowledgement did not clear pending message");
    depart(*a,0,"diplomacy");check(a->residents[0].away&&a->outbox.size()==1,"Sending a visitor failed to suspend the original resident");
    check(a->save("active-outgoing-test.json")&&restored->load("active-outgoing-test.json")&&restored->residents[0].away&&restored->outbox.size()==1,"Outgoing resident duplicated after restart");
    const std::string id=a->visits[0].id,payload=a->outbox.at(id);deliver(client,host,*b,a->worldId,payload);deliver(client,host,*b,a->worldId,payload);
    check(b->residents.size()==7&&b->visits.size()==1&&b->residents.back().visitor,"Repeated packet duplicated visiting resident");
    check(b->save("visitor-test.json")&&restored->load("visitor-test.json"),"Incoming visit cannot survive save/load");
    restored->updateVisits(45);check(restored->outbox.count("result:"+id)==1&&restored->treasury==97,"Receiving island did not resolve diplomacy exactly once");
    std::string result=restored->outbox.at("result:"+id);int before=a->residents[0].money;
    a->terrain[a->ships[0].cell]=isla::Terrain::Mountain;
    deliver(host,client,*a,b->worldId,result);deliver(host,client,*a,b->worldId,result);
    check(a->residents[0].away&&a->ships[0].state=="visit_returning","Result teleported visitor home instead of sailing");returnHome(*a,0);
    check(a->walkable(static_cast<int>(a->residents[0].x),static_cast<int>(a->residents[0].y)),"Returning visitor landed inside a new obstacle");
    check(a->relations[b->worldId]==10&&restored->relations[a->worldId]==10&&!a->residents[0].away&&a->residents[0].money==before+3&&a->metrics["visits_returned"]==1,"Return repeated or duplicated diplomatic resources");
    deliver(client,host,*restored,a->worldId,a->outbox.at("ack:"+id));check(!restored->outbox.count("result:"+id),"Result acknowledgement did not complete delivery");
    a->outbox.clear();depart(*a,1,"raid");std::string raid=a->visits.back().id;deliver(client,host,*restored,a->worldId,a->outbox.at(raid));int targetGold=restored->treasury,targetFood=restored->grain;restored->updateVisits(45);
    check(restored->treasury==targetGold-12&&restored->grain==targetFood-8,"Raid did not debit resources on the receiving island");
    result=restored->outbox.at("result:"+raid);deliver(host,client,*a,b->worldId,result);returnHome(*a,1);check(a->relations[b->worldId]==-10&&a->residents[1].money==24&&a->residents[1].grain==8,"Raid spoils did not return to their actual owner");
    check(a->save("outgoing-test.json")&&restored->load("outgoing-test.json")&&restored->worldId==a->worldId,"Island identity and completed visits did not persist");
    check(client.join("127.0.0.1",port),"Cannot reconnect after replacing a TCP connection");
    for(int i=0;i<1000;++i){host.poll();client.poll();if(host.connected()&&client.connected()&&i>10)break;std::this_thread::sleep_for(std::chrono::milliseconds(1));}
    check(host.connected()&&client.connected(),"TCP session did not recover on reconnect");
    int returned=a->metrics["visits_returned"];deliver(host,client,*a,b->worldId,result);check(a->metrics["visits_returned"]==returned,"Reconnect replay duplicated a completed visit");
    auto altered=json::parse(result);altered["food"]=-10;check(a->receivePeer(altered.dump(),std::string(32,'f'))=="Destino de red incorrecto","Spoofed peer message was accepted");
    std::cout<<"PASS: nine-month birth deadline, newborn, pregnancy save/load, rain, gift collision/collection, autonomous needs, two TCP peers, diplomacy, raid, duplicate delivery, return and persistence\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}





