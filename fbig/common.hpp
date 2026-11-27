#ifndef FBIG_COMMON_HPP
#define FBIG_COMMON_HPP

#include "config.hpp"
#include <cstdint>

namespace fbig {

struct U128 { uint64_t hi, lo; };

inline U128 mul64x64(uint64_t a, uint64_t b) {
#if FBIG_HAS_INT128
    unsigned __int128 p = (unsigned __int128)a * b;
    return { (uint64_t)(p >> 64), (uint64_t)p };
#else
    uint64_t u1 = a & 0xFFFFFFFF, v1 = b & 0xFFFFFFFF;
    uint64_t u2 = a >> 32, v2 = b >> 32;
    uint64_t t = u1 * v1;
    uint64_t w3 = t & 0xFFFFFFFF;
    uint64_t k = t >> 32;
    t = u2 * v1 + k;
    k = t & 0xFFFFFFFF;
    uint64_t w1 = t >> 32;
    t = u1 * v2 + k;
    k = t >> 32;
    return { u2 * v2 + w1 + k, (k << 32) + w3 };
#endif
}

inline U128 udiv128(U128 n, uint64_t d) {
#if FBIG_HAS_INT128
    unsigned __int128 num = ((unsigned __int128)n.hi << 64) | n.lo;
    return { (uint64_t)(num / d), (uint64_t)(num % d) };
#else
    uint64_t q = 0, r = 0;
    for (int bit = 63; bit >= 0; --bit) {
        r = (r << 1) | ((n.hi >> bit) & 1);
        if (r >= d) { r -= d; q |= (1ULL << bit); }
    }
    return { q, r };
#endif
}

inline int clz64(uint64_t x) {
    if (x == 0) return 64;
#if FBIG_HAS_BUILTIN_BIT
    return __builtin_clzll(x);
#else
    int n = 0;
    while (!(x & (1ULL << 63))) { x <<= 1; ++n; }
    return n;
#endif
}

} // namespace fbig
#endif // FBIG_COMMON_HPP
