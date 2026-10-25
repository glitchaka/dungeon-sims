#pragma once
#include "entities/Task.h"
#include <vector>
namespace isla {
class World;struct Resident;
struct ResidentAction {Task task{Task::Idle};int goal{-1},target{-1},workCell{-1},job{-1},boat{-1};bool family{};float cost{},risk{};};
class ResidentActionCatalog {public:static std::vector<ResidentAction> available(const World&,const Resident&);};
}
