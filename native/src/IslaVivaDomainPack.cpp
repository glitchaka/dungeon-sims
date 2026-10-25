#include "IslaVivaDomainPack.h"
#include "entities/Task.h"
#include <mspa/semantics/domain_pack.hpp>
#include <stdexcept>
namespace isla {
mspa::RegistrySnapshot islaVivaRegistry(){
    using namespace mspa;DomainPack pack;pack.metadata.id=PackId(1);pack.metadata.name_space=NamespaceId(1);pack.metadata.name="Isla Viva";
    const char* concepts[]={"sed","hambre","descanso","compan ia","diversion","higiene","comunidad","familia","seguridad"};
    for(int i=0;i<9;++i)pack.concepts.push_back({ConceptId(i+1),concepts[i]});
    for(int i=0;i<=static_cast<int>(Task::Fish);++i)pack.action_types.push_back({ActionTypeId(i+1),taskName(static_cast<Task>(i))});
    pack.action_types.push_back({ActionTypeId(100),"Formar una familia"});
    for(int i=1;i<=6;++i){PredicateSchema p;p.id=PredicateId(i);p.name_space=NamespaceId(1);p.required_roles={"actor"};p.agency_role="actor";p.default_relevance=.75f;p.default_consequence=i==5?.4f:i==6?.6f:-.6f;p.related_concepts={ConceptId(i<=3?i:9)};pack.predicates.push_back(p);}
    pack.initial_values={{ConceptId(7),.5f},{ConceptId(8),.65f},{ConceptId(9),.8f}};
    RegistryBuilder builder;auto applied=apply_domain_pack(builder,pack);if(!applied)throw std::runtime_error(applied.error_if()->details);return builder.publish();
}
}
