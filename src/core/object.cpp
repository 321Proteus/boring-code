
#include "object.hpp"
#include "address.hpp"
#include "view.hpp"
#include <vector>

void BCBlock::dispatch_details(const BCDetailsViewModel& vm) const { vm.show_details(*this); }
void BCBasicBlock::dispatch_details(const BCDetailsViewModel& vm) const { vm.show_details(*this); }
void BCLoop::dispatch_details(const BCDetailsViewModel& vm) const { vm.show_details(*this); }

std::vector<BCAddr> BCBasicBlock::get_code_addrs() const {
    return { address };
}

std::vector<BCAddr> BCBlock::get_code_addrs() const {
    std::vector<uint64_t> addrs;
    for (BCObject* m : this->members) {
        auto member_addrs = m->get_code_addrs();
        addrs.insert(addrs.end(), member_addrs.begin(), member_addrs.end());
    }
    return addrs;
}

std::vector<BCAddr> BCLoop::get_code_addrs() const {
    std::vector<uint64_t> addrs;
    for (BCObject* m : this->body) {
        auto member_addrs = m->get_code_addrs();
        addrs.insert(addrs.end(), member_addrs.begin(), member_addrs.end());
    }
    return addrs;
}