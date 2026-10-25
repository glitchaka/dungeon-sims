#include "ResidentMindSystem.h"
#include "ResidentActionCatalog.h"
#include "IslaVivaDomainPack.h"
#include "World.h"
#include <mspa/mspa.hpp>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>

namespace isla {
using nlohmann::json;
namespace {
std::uint64_t identity(const std::string& text){std::uint64_t n=1469598103934665603ull;for(unsigned char c:text)n=(n^c)*1099511628211ull;return n?n:1;}
template<class T> void require(const T& result){if(!result)throw std::runtime_error(result.error_if()->details);}
std::uint64_t typeOf(const ResidentAction& a){return a.family?100:static_cast<std::uint64_t>(a.task)+1;}
}
struct ResidentMindSystem::State {
    std::string world;std::uint64_t seed{},ticks{},lastAdvance{},sequence{};
    std::map<int,std::vector<std::uint8_t>> saved;
    struct Active{Task task;std::uint64_t type;int target;std::vector<std::string> tags;};
    std::map<int,Active> active;std::map<int,std::string> explanations;std::set<int> retired;
    std::map<int,std::array<int,4>> bands;
    std::unique_ptr<mspa::Runtime> runtime;
    State()=default;
    State(const State& other):world(other.world),seed(other.seed),ticks(other.ticks),lastAdvance(other.lastAdvance),sequence(other.sequence),saved(other.saved),active(other.active),explanations(other.explanations),retired(other.retired),bands(other.bands){}
    mspa::PsychTime now() const{return mspa::PsychTime(ticks);}
    mspa::AgentId id(int index)const{return {mspa::WorldId(identity(world)),static_cast<std::uint64_t>(index)+1};}
    void flush(int index){if(!runtime||!runtime->has_agent(id(index)))return;auto bytes=runtime->export_agent_bytes(id(index),now());require(bytes);saved[index]=std::move(*bytes.value_if());}
    void ensure(World& w){
        using namespace mspa;
        if(world.empty()){world=w.worldId;seed=w.islandSeed;ticks=static_cast<std::uint64_t>(std::max(0.0,w.elapsed)*1000000.0);}
        if(world!=w.worldId)throw std::runtime_error("Psychological state belongs to another island");
        for(const auto& agent:saved)if(agent.first<0||agent.first>=static_cast<int>(w.residents.size()))throw std::runtime_error("Psychological state refers to an unknown resident");
        if(!runtime){RuntimeConfig config;config.runtime_id=RuntimeId(identity(world));config.world_id=WorldId(identity(world));config.time_domain={TimeDomainId(identity(world)),{1000000,1}};config.seed_root=seed;
            config.limits.max_agents=1000;config.limits.max_beliefs_per_agent=64;config.limits.max_memories_per_agent=32;config.limits.max_goals_per_agent=16;config.limits.max_relationships_per_agent=1000;config.limits.max_learning_patterns_per_agent=128;config.limits.max_diagnostics=512;config.limits.max_serialized_bytes=131072;
            auto created=Runtime::create(config,islaVivaRegistry());require(created);runtime=std::move(*created.value_if());
            for(const auto& entry:saved)if(!retired.count(entry.first)){ImportOptions options;options.destination_id=id(entry.first);options.import_time=now();options.time_policy=ImportTimePolicy::PreserveAbsolute;options.rng_policy=RngImportPolicy::PreserveState;require(runtime->import_agent_bytes(entry.second,options));}
        }
        for(std::size_t i=0;i<w.residents.size();++i){const int index=static_cast<int>(i);const auto& r=w.residents[i];
            if(!r.alive){if(runtime->has_agent(id(index))){flush(index);require(runtime->remove_agent(id(index),now()));}retired.insert(index);active.erase(index);continue;}
            if(runtime->has_agent(id(index)))continue;
            retired.erase(index);
            if(r.mother>=0&&r.father>=0&&runtime->has_agent(id(r.mother))&&runtime->has_agent(id(r.father))){DerivedInit init;init.source_agents={id(r.mother),id(r.father)};init.inherited_value_policy=InheritedValuePolicy::Mean;init.variation_seed=static_cast<std::uint64_t>(index);init.variation_strength=.12f;require(runtime->create_agent_derived(id(index),init,now()));}
            else{AgentInit init;init.use_registry_default_values=true;init.personality.sociability=r.trait=="Sociable"?.85f:.5f;init.personality.curiosity=r.trait.find("Explorador")==0?.85f:.45f;init.personality.boldness=r.trait=="Cazadora"?.8f:.45f;init.personality.persistence=r.trait=="Trabajador"?.8f:.55f;
                for(int concept=1;concept<=3;++concept){GoalState goal;goal.id=GoalId(concept);goal.concept_id=ConceptId(concept);goal.priority=.8f;goal.commitment=.7f;init.goals.push_back(goal);}
                require(runtime->create_agent(id(index),init,now()));}
            flush(index);
        }
    }
    void observe(int index,int predicate,const std::string& qualifier,bool positive=true){
        using namespace mspa;ObserveInput input;input.agent=id(index);input.time.process_time=now();input.proposition.predicate=PredicateId(predicate);input.proposition.name_space=NamespaceId(1);input.proposition.positive=positive;
        PropositionArg actor;actor.role="actor";actor.has_agent=true;actor.agent=id(index);input.proposition.arguments.push_back(actor);input.proposition.qualifiers["experience"]=qualifier;
        input.source=predicate==5?BeliefSource::Influence:BeliefSource::Observation;
        const auto result=runtime->observe(input);if(!result&&result.error_if()->code!=ErrorCode::QuotaExceeded)require(result);
    }
};
ResidentMindSystem::ResidentMindSystem():state_(std::make_unique<State>()){}
ResidentMindSystem::~ResidentMindSystem()=default;
ResidentMindSystem::ResidentMindSystem(const ResidentMindSystem& other):state_(other.state_?std::make_unique<State>(*other.state_):std::make_unique<State>()){}
ResidentMindSystem& ResidentMindSystem::operator=(const ResidentMindSystem& other){if(this!=&other){ResidentMindSystem copy(other);state_.swap(copy.state_);}return *this;}
ResidentMindSystem::ResidentMindSystem(ResidentMindSystem&&) noexcept=default;
ResidentMindSystem& ResidentMindSystem::operator=(ResidentMindSystem&&) noexcept=default;

void ResidentMindSystem::advance(World& w,float dt){
    auto& s=*state_;s.ensure(w);s.ticks+=static_cast<std::uint64_t>(std::llround(static_cast<double>(dt)*1000000));
    for(auto it=s.active.begin();it!=s.active.end();){const int index=it->first;const auto active=it->second;++it;
        if(index>=static_cast<int>(w.residents.size())||!w.residents[index].alive)continue;
        const auto& r=w.residents[index];if(!r.away&&!r.inBoat&&r.task!=active.task)complete(w,index,false,"interrupted");}
    if(s.ticks-s.lastAdvance<5000000)return;s.lastAdvance=s.ticks;
    for(std::size_t i=0;i<w.residents.size();++i){const auto& r=w.residents[i];if(!r.alive||r.away)continue;const int index=static_cast<int>(i);
        require(s.runtime->advance_to(s.id(index),s.now()));
        const std::array<int,4> band{{r.thirst<25?1:0,r.hunger<25?1:0,r.energy<25?1:0,w.alarmSeconds>0&&std::hypot(r.x-w.alarmX,r.y-w.alarmY)<cnc::fleeRadius?1:0}};
        const auto prior=s.bands.find(index);for(int k=0;k<4;++k)if(prior==s.bands.end()||prior->second[k]!=band[k])s.observe(index,k+1,k==3?"danger":"body",band[k]!=0);
        s.bands[index]=band;s.flush(index);
    }
}
void ResidentMindSystem::complete(World& w,int index,bool success,const std::string& outcome){
    auto& s=*state_;s.ensure(w);auto found=s.active.find(index);if(found==s.active.end()||!s.runtime->has_agent(s.id(index)))return;
    mspa::ReportInput report;report.agent=s.id(index);report.action_type=mspa::ActionTypeId(found->second.type);report.success=success;report.process_time=s.now();report.context_tags=found->second.tags;report.outcome_tags={outcome};
    if(success&&found->second.target>=0&&found->second.target<static_cast<int>(w.residents.size()))report.relationship_impacts.push_back({s.id(found->second.target),{.03f,.02f,.01f,.04f}});
    require(s.runtime->report(report));s.active.erase(found);s.flush(index);
}
void ResidentMindSystem::influence(World& w,int index,const std::string& meaning){auto& s=*state_;s.ensure(w);if(s.runtime->has_agent(s.id(index))){s.observe(index,5,meaning);s.flush(index);}}

bool ResidentMindSystem::plan(World& w,Resident& r){
    using namespace mspa;auto& s=*state_;s.ensure(w);const int index=w.residents.indexOf(r);if(index<0||!r.alive)return false;
    if(r.insideHouse>=0){if(w.night()&&r.hunger>=18&&r.thirst>=20&&w.alarmSeconds<=0){r.task=Task::Rest;return true;}if(!w.leaveShelter(r))return true;}
    if(r.familyPartner>=0&&r.next<r.path.size()&&w.elapsed<r.familySeekAfter&&r.hunger>=18&&r.thirst>=20&&!w.night())return true;
    auto candidates=ResidentActionCatalog::available(w,r);
    DecisionContext context;const float needs[]={r.thirst,r.hunger,r.energy,r.social,r.fun,r.hygiene};
    for(int i=0;i<6;++i)context.needs.push_back({ConceptId(i+1),std::clamp((100-needs[i])/100,0.0f,1.0f)});
    context.needs.push_back({ConceptId(7),w.grain<w.reserve?1.0f:.35f});context.needs.push_back({ConceptId(8),.55f});
    const bool danger=w.alarmSeconds>0&&std::hypot(r.x-w.alarmX,r.y-w.alarmY)<cnc::fleeRadius;
    context.needs.push_back({ConceptId(9),danger?1.0f:0.0f});context.urgency=(r.thirst<20||r.hunger<18||danger)?1.0f:0.0f;
    const auto id=s.id(index);std::vector<ActionOption> options;
    for(std::size_t i=0;i<candidates.size();++i){const auto& c=candidates[i];ActionOption option;option.actor=id;option.type=ActionTypeId(typeOf(c));option.instance=ActionInstanceId(++s.sequence);option.continuity=ActionContinuityKey(typeOf(c));option.cost=c.cost;option.risk=c.risk;option.context_tags={w.night()?"night":"day",w.raining?"rain":"dry"};
        const auto effect=[&](int concept,float magnitude){option.expected_effects.push_back({EffectChannel::Need,ConceptId(concept),{},magnitude,{}});};
        switch(c.task){case Task::Drink:case Task::DrinkHome:effect(1,1);break;case Task::Eat:effect(2,1);break;case Task::Fetch:effect(2,.8f);break;
        case Task::Rest:effect(3,1);if(w.night())effect(9,.8f);break;case Task::Social:effect(4,1);option.social_exposure=1;break;
        case Task::Bathe:effect(6,1);break;case Task::Wander:effect(5,.7f);option.novelty=.8f;break;case Task::Flee:effect(9,1);effect(9,1);break;
        case Task::Harvest:effect(2,.8f);effect(7,1);break;case Task::Sow:case Task::Till:effect(7,.8f);break;
        case Task::Gather:effect(7,.55f);break;case Task::Deposit:effect(7,1);break;case Task::Project:effect(7,.9f);break;
        case Task::Hunt:case Task::Tend:case Task::Fish:effect(2,.7f);effect(7,.7f);break;case Task::Trade:case Task::SellLoot:effect(7,.8f);break;
        case Task::Camp:effect(3,.6f);break;case Task::Expedition:effect(5,.8f);option.novelty=1;break;default:option.cost+=.5f;break;}
        if(c.family){effect(8,1);option.expected_effects.push_back({EffectChannel::Value,ConceptId(8),{},.4f,{}});}
        if(c.target>=0){option.targets.push_back(s.id(c.target));option.expected_effects.push_back({EffectChannel::Relationship,{},s.id(c.target),.4f,{}});}
        if((r.thirst<20&&c.task!=Task::Drink&&c.task!=Task::DrinkHome)||(r.hunger<18&&c.task!=Task::Eat&&c.task!=Task::Fetch&&c.task!=Task::Harvest&&c.task!=Task::Hunt&&c.task!=Task::Tend))option.cost+=2;
        options.push_back(std::move(option));
    }
    while(!options.empty()){
        auto chosen=s.runtime->decide(id,options,context,s.now());require(chosen);if(!chosen.value_if()->has_selection)return false;
        const auto selected=std::find_if(options.begin(),options.end(),[&](const ActionOption& option){return option.instance==chosen.value_if()->selected_action;});if(selected==options.end())throw std::runtime_error("MSPA selected an unavailable action");
        const std::size_t i=static_cast<std::size_t>(selected-options.begin());const auto c=candidates[i];
        complete(w,index,false,"replanned");
        bool started=false;
        if(c.family)started=w.seekFamily(r);
        else if(c.boat>=0){w.startFishing(index,c.boat);started=r.hasOrder&&r.orderedTask==Task::Fish;}
        else if(c.task==Task::Idle){r.task=Task::Idle;r.path.clear();r.next=0;r.decisionTimer=2;started=true;}
        else{started=w.begin(r,c.task,c.goal);if(started){r.target=c.target;if(c.workCell>=0)r.workCell=c.workCell;if(c.job>=0){r.job=c.job;for(auto& job:w.jobs)if(job.id==c.job)job.worker=index;}}}
        if(started){s.active[index]={r.task,typeOf(c),c.target,selected->context_tags};s.explanations[index]=c.family?"Busca formar una familia":World::taskName(c.task);s.flush(index);++w.metrics["mspa_decisions"];return true;}
        options.erase(options.begin()+i);candidates.erase(candidates.begin()+i);
    }
    return false;
}

std::vector<std::uint8_t> ResidentMindSystem::exportAgent(World& w,int index){state_->ensure(w);state_->flush(index);const auto it=state_->saved.find(index);return it==state_->saved.end()?std::vector<std::uint8_t>{}:it->second;}
bool ResidentMindSystem::importAgent(World& w,int index,const std::vector<std::uint8_t>& bytes,bool returning){
    using namespace mspa;auto& s=*state_;s.ensure(w);SerializationOptions bounds;bounds.max_bytes=131072;auto decoded=deserialize_agent(bytes,bounds);if(!decoded)return false;
    const auto old=decoded.value_if()->state.id,destination=s.id(index);
    auto remap=[&](AgentId& id){if(id==old)id=destination;};
    auto& agent=decoded.value_if()->state;
    for(auto& b:agent.beliefs){remap(b.source_agent);for(auto& a:b.proposition.arguments)if(a.has_agent)remap(a.agent);}
    for(auto& memory:agent.memories){for(auto& p:memory.participants)remap(p);for(auto& a:memory.proposition.arguments)if(a.has_agent)remap(a.agent);}
    for(auto& relationship:agent.relationships)remap(relationship.other);
    ImportOptions options;options.destination_id=destination;options.import_time=s.now();options.destination_seed_root=s.seed;options.rng_policy=returning?RngImportPolicy::PreserveState:RngImportPolicy::RekeyForDestination;
    if(s.runtime->has_agent(destination))require(s.runtime->remove_agent(destination,s.now()));
    auto imported=s.runtime->import_agent(*decoded.value_if(),options);if(!imported){s.runtime.reset();return false;}s.retired.erase(index);s.flush(index);s.active.erase(index);return true;
}
std::string ResidentMindSystem::save()const{const auto& s=*state_;json active=json::array();for(const auto& a:s.active)active.push_back({a.first,static_cast<int>(a.second.task),a.second.type,a.second.target,a.second.tags});return json{{"version",1},{"pack",1},{"world",s.world},{"seed",s.seed},{"ticks",s.ticks},{"lastAdvance",s.lastAdvance},{"sequence",s.sequence},{"agents",s.saved},{"retired",s.retired},{"active",active}}.dump();}
void ResidentMindSystem::load(const std::string& encoded){const auto data=json::parse(encoded);if(data.at("version")!=1||data.at("pack")!=1)throw std::runtime_error("Incompatible Isla Viva psychology pack");auto next=std::make_unique<State>();next->world=data.at("world");next->seed=data.at("seed");next->ticks=data.at("ticks");next->lastAdvance=data.at("lastAdvance");next->sequence=data.at("sequence");data.at("agents").get_to(next->saved);data.at("retired").get_to(next->retired);if(next->saved.size()>1000||next->lastAdvance>next->ticks)throw std::runtime_error("Invalid psychological snapshot");for(const auto& entry:next->saved)if(entry.first<0||entry.first>=1000||entry.second.size()>131072)throw std::runtime_error("Invalid psychological agent snapshot");for(const auto& a:data.at("active")){const int index=a.at(0);if(index<0||index>=1000)throw std::runtime_error("Invalid active psychological action");next->active[index]={static_cast<Task>(a.at(1).get<int>()),a.at(2).get<std::uint64_t>(),a.at(3).get<int>(),a.at(4).get<std::vector<std::string>>()};}state_=std::move(next);}
std::string ResidentMindSystem::explanation(int index)const{auto it=state_->explanations.find(index);return it==state_->explanations.end()?std::string{}:it->second;}
}
