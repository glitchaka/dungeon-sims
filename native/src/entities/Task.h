#pragma once
namespace isla {
enum class Task {Idle,Wander,Drink,Eat,Rest,Flee,Bathe,Social,Gather,Camp,Trade,Till,Sow,Harvest,Deposit,Fetch,Hunt,Tend,DrinkHome,Project,Expedition,SellLoot,Move,Fish};
const char* taskName(Task task);
}

