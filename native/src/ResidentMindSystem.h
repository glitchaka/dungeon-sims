#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
namespace isla {
class World;struct Resident;
// Owns psychology only. Copying a World copies serialized minds, never shares a mutable runtime.
class ResidentMindSystem {
public:
    ResidentMindSystem();~ResidentMindSystem();
    ResidentMindSystem(const ResidentMindSystem&);ResidentMindSystem& operator=(const ResidentMindSystem&);
    ResidentMindSystem(ResidentMindSystem&&) noexcept;ResidentMindSystem& operator=(ResidentMindSystem&&) noexcept;
    bool plan(World&,Resident&);
    void advance(World&,float);
    void complete(World&,int,bool,const std::string&);
    void influence(World&,int,const std::string&);
    std::string save() const;void load(const std::string&);
    std::vector<std::uint8_t> exportAgent(World&,int);
    bool importAgent(World&,int,const std::vector<std::uint8_t>&,bool returning=false);
    std::string explanation(int) const;
private:
    struct State;std::unique_ptr<State> state_;
};
}
