#include <array>
#include <cstdint>
#include <cstring>

static constexpr std::array<uint64_t, 5> MULTIPLIERS = {
    2654435761ULL,
    2246822519ULL,
    3266489917ULL,
    668265263ULL,
    374761393ULL
};

static constexpr uint64_t MOD = (1ULL << 61) - 1;

inline uint64_t modm(uint64_t a) {
    a = (a >> 61) + (a & MOD);
    if (a >= MOD) a -= MOD;
    return a;
}

inline uint64_t hash_window(const uint32_t* ptr, int w) {
    uint64_t h = 0;
    for (int i=0;i<w;i++)
        h = modm(h + static_cast<uint64_t>(ptr[i]) * MULTIPLIERS[i]);
    return h;
}

inline bool verify_window(const uint32_t* a, const uint32_t* b, int w) {
    return memcmp(a, b, w * sizeof(uint32_t)) == 0;
}