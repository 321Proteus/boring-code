#include <cstdint>
#include <cstring>

static constexpr uint64_t MOD = (1ULL << 61) - 1;

inline uint64_t modm(uint64_t a) {
    a = (a >> 61) + (a & MOD);
    if (a >= MOD) a -= MOD;
    return a;
}

inline uint64_t hash_window(const uint32_t* ptr, int w) {
    uint64_t h = 0;
    for (int i=0;i<w;i++)
        h = modm(h * 1315423911ULL + static_cast<uint64_t>(ptr[i]));
    return h;
}

template <typename Iterator>
uint64_t hash_window(Iterator begin, Iterator end) {
    uint64_t h = 0;
    for (Iterator i=begin;i!=end;i++) {
        h = modm(h * 1315423911ULL + static_cast<uint64_t>(*i));
    }
    return h;
}

inline bool verify_window(const uint32_t* a, const uint32_t* b, int w) {
    return memcmp(a, b, w * sizeof(uint32_t)) == 0;
}