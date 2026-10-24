#pragma once
#include "entities/Resident.h"
#include <utility>
#include <vector>

namespace isla {
// Append-only: saved indices, parentage and active jobs keep their identity.
// Population changes go through add(), including births and network visitors.
class ResidentRegistry {
public:
    int add(Resident resident);
    void restore(std::vector<Resident> residents);

    std::size_t size() const noexcept { return residents_.size(); }
    bool empty() const noexcept { return residents_.empty(); }
    Resident& operator[](std::size_t index) { return residents_[index]; }
    const Resident& operator[](std::size_t index) const { return residents_[index]; }
    Resident& back() { return residents_.back(); }
    const Resident& back() const { return residents_.back(); }
    int indexOf(const Resident& resident) const noexcept {
        return static_cast<int>(&resident - residents_.data());
    }
    auto begin() noexcept { return residents_.begin(); }
    auto end() noexcept { return residents_.end(); }
    auto begin() const noexcept { return residents_.begin(); }
    auto end() const noexcept { return residents_.end(); }
    const std::vector<Resident>& values() const noexcept { return residents_; }

private:
    std::vector<Resident> residents_;
};
}

