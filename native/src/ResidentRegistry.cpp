#include "ResidentRegistry.h"
#include <algorithm>
#include <stdexcept>

namespace isla {
int ResidentRegistry::add(Resident resident) {
    if (residents_.size() >= 1000) throw std::length_error("Resident limit exceeded");
    const int index = static_cast<int>(residents_.size());
    resident.relationships.resize(residents_.size() + 1);
    // Prepare relationship growth before committing population membership.
    // Keep geometric capacity growth instead of reallocating every row per birth.
    for (auto& existing : residents_) {
        auto& relationships = existing.relationships;
        if (relationships.capacity() < residents_.size() + 1)
            relationships.reserve(std::max(residents_.size() + 1, relationships.capacity() * 2));
    }
    residents_.push_back(std::move(resident));
    for (auto& existing : residents_) existing.relationships.resize(residents_.size());
    return index;
}

void ResidentRegistry::restore(std::vector<Resident> residents) {
    if (residents.size() > 1000) throw std::length_error("Resident limit exceeded");
    for (auto& resident : residents) resident.relationships.resize(residents.size());
    residents_ = std::move(residents);
}
}
