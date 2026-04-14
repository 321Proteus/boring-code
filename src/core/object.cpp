
#include "object.hpp"
#include "ui/view.hpp"

void BCBlock::dispatch_details(const BCDetailsViewModel& vm) const { vm.show_details(*this); }
void BCBasicBlock::dispatch_details(const BCDetailsViewModel& vm) const { vm.show_details(*this); }
void BCLoop::dispatch_details(const BCDetailsViewModel& vm) const { vm.show_details(*this); }