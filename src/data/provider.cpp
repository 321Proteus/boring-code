#include "data/provider.hpp"
#include "core/address.hpp"
#include "core/object.hpp"
#include <algorithm>
#include <iterator>

BCCodeProvider* BCCodeProviderRegistry::find(BCAddr addr) const {
    if (last_index >= 0 && addr >= mapping[last_index].start && addr < mapping[last_index].end)
        return mapping[last_index].provider.get();

    auto it = std::upper_bound(
        mapping.begin(), mapping.end(), addr,
        [](BCAddr addr, const Entry& e) {
            return addr < e.start;
    });

    if (it == mapping.begin()) return nullptr;
    it--;
    if (addr >= it->start && addr < it->end) {
        last_index = std::distance(mapping.begin(), it);
        return it->provider.get();
    }
    return nullptr;
}

void BCCodeProviderRegistry::register_provider(BCModule mod, std::unique_ptr<BCCodeProvider> prov) {
    auto pos = std::lower_bound(
        mapping.begin(), mapping.end(), mod.start,
        [](const Entry& e, BCAddr addr) {
            return e.start < addr;
    });
    Entry e { mod.start, mod.end, std::move(prov) };
    mapping.insert(pos, std::move(e));
}
