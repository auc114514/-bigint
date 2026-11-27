#ifndef FBIG_HPP
#define FBIG_HPP

#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>
#include <stdexcept>
#include <cctype>
#include <random>
#include <numeric>
#include <cstdio>

namespace fbig {

// ==========================================
// 1. 底层跨平台原语
// ==========================================
struct U128 { uint64_t hi, lo; };

inline U128 mul64x64(uint64_t a, uint64_t b) {
#if defined(__SIZEOF_INT128__)
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
#if defined(__SIZEOF_INT128__)
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
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_clzll(x);
#else
    int n = 0;
    while (!(x & (1ULL << 63))) { x <<= 1; ++n; }
    return n;
#endif
}

// ==========================================
// 2. 乘法引擎 (NTT, Karatsuba, Base)
// ==========================================
constexpr size_t KARATSUBA_THRESHOLD = 32;
constexpr size_t NTT_THRESHOLD = 512;

constexpr uint64_t NTT_MOD = 998244353;
constexpr uint64_t NTT_ROOT = 3;

inline uint64_t mod_pow(uint64_t base, uint64_t exp, uint64_t mod) {
    uint64_t res = 1;
    base %= mod;
    while (exp) {
        if (exp & 1) res = (unsigned __int128)res * base % mod;
        base = (unsigned __int128)base * base % mod;
        exp >>= 1;
    }
    return res;
}

inline void ntt(std::vector<uint64_t>& a, bool invert) {
    size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i) {
        size_t bit = n >> 1;
        while (j & bit) { j ^= bit; bit >>= 1; }
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }
    for (size_t len = 2; len <= n; len <<= 1) {
        uint64_t wlen = mod_pow(NTT_ROOT, (NTT_MOD - 1) / len, NTT_MOD);
        if (invert) wlen = mod_pow(wlen, NTT_MOD - 2, NTT_MOD);
        for (size_t i = 0; i < n; i += len) {
            uint64_t w = 1;
            for (size_t j = 0; j < len / 2; ++j) {
                uint64_t u = a[i+j];
                uint64_t v = (unsigned __int128)a[i+j+len/2] * w % NTT_MOD;
                a[i+j] = (u + v) % NTT_MOD;
                a[i+j+len/2] = (u - v + NTT_MOD) % NTT_MOD;
                w = (unsigned __int128)w * wlen % NTT_MOD;
            }
        }
    }
    if (invert) {
        uint64_t n_inv = mod_pow(n, NTT_MOD - 2, NTT_MOD);
        for (uint64_t& x : a) x = (unsigned __int128)x * n_inv % NTT_MOD;
    }
}

inline std::vector<uint64_t> ntt_multiply(const std::vector<uint64_t>& a, const std::vector<uint64_t>& b) {
    if (a.empty() || b.empty()) return {};
    if (a.size() + b.size() > NTT_MOD / (255 * 255)) throw std::runtime_error("ntt_multiply: too large");

    std::vector<uint64_t> A, B;
    A.reserve(a.size() * 8); B.reserve(b.size() * 8);
    for (uint64_t limb : a) for (int i = 0; i < 8; ++i) A.push_back((limb >> (i * 8)) & 0xFF);
    for (uint64_t limb : b) for (int i = 0; i < 8; ++i) B.push_back((limb >> (i * 8)) & 0xFF);

    size_t n = 1;
    while (n < A.size() + B.size()) n <<= 1;
    A.resize(n, 0); B.resize(n, 0);

    ntt(A, false); ntt(B, false);
    for (size_t i = 0; i < n; ++i) A[i] = (unsigned __int128)A[i] * B[i] % NTT_MOD;
    ntt(A, true);

    std::vector<uint64_t> res(a.size() + b.size() + 2, 0);
    unsigned __int128 carry = 0;
    for (size_t i = 0; i < A.size(); ++i) {
        unsigned __int128 cur = (unsigned __int128)A[i] + carry;
        size_t limb_idx = i / 8;
        size_t bit_shift = (i % 8) * 8;
        uint64_t chunk = (uint64_t)(cur & 0xFF);
        if (limb_idx < res.size()) res[limb_idx] |= (chunk << bit_shift);
        if (bit_shift > 56 && limb_idx + 1 < res.size()) res[limb_idx + 1] |= (chunk >> (64 - bit_shift));
        carry = cur >> 8;
    }
    for (size_t i = 0; i < res.size() && carry; ++i) {
        unsigned __int128 cur = (unsigned __int128)res[i] + carry;
        res[i] = (uint64_t)cur;
        carry = cur >> 64;
    }
    while (!res.empty() && res.back() == 0) res.pop_back();
    return res;
}

inline void base_mul(const uint64_t* a, size_t na, const uint64_t* b, size_t nb, uint64_t* out) {
    for (size_t i = 0; i < na + nb; ++i) out[i] = 0;
    for (size_t i = 0; i < na; ++i) {
        if (!a[i]) continue;
        uint64_t carry_lo = 0, carry_hi = 0;
        for (size_t j = 0; j < nb; ++j) {
            U128 p = mul64x64(a[i], b[j]);
            uint64_t s1 = out[i+j] + p.lo;
            uint64_t c1 = (s1 < p.lo) ? 1 : 0;
            uint64_t s2 = s1 + carry_lo;
            uint64_t c2 = (s2 < s1) ? 1 : 0;
            uint64_t next_lo = p.hi + carry_hi;
            uint64_t next_hi = (next_lo < p.hi) ? 1 : 0;
            next_lo += c1;
            next_hi += (next_lo < c1) ? 1 : 0;
            next_lo += c2;
            next_hi += (next_lo < c2) ? 1 : 0;
            out[i+j] = s2;
            carry_lo = next_lo;
            carry_hi = next_hi;
        }
        out[i+nb] += carry_lo;
    }
}

inline void karatsuba_mul(const uint64_t* a, const uint64_t* b, uint64_t* out, size_t n, uint64_t* scratch) {
    if (n <= KARATSUBA_THRESHOLD) { base_mul(a, n, b, n, out); return; }
    size_t half = n / 2;
    uint64_t* a0 = scratch;            uint64_t* a1 = scratch + half;
    uint64_t* b0 = scratch + 2 * half; uint64_t* b1 = scratch + 3 * half;
    uint64_t* z0 = scratch + 4 * half; uint64_t* z2 = scratch + 6 * half;
    uint64_t* z1 = scratch + 8 * half;

    for (size_t i = 0; i < half; ++i) { a0[i] = a[i]; b0[i] = b[i]; }
    for (size_t i = half; i < n; ++i) { a1[i - half] = a[i]; b1[i - half] = b[i]; }

    base_mul(a0, half, b0, half, z0);
    base_mul(a1, half, b1, half, z2);

    uint64_t* sum_a = scratch + 10 * half;
    uint64_t* sum_b = scratch + 11 * half;
    for (size_t i = 0; i < half; ++i) {
        sum_a[i] = a0[i] + a1[i];
        sum_b[i] = b0[i] + b1[i];
    }
    base_mul(sum_a, half, sum_b, half, z1);
    for (size_t i = 0; i < 2 * half; ++i) {
        z1[i] -= z0[i];
        z1[i] -= z2[i];
    }

    for (size_t i = 0; i < 2 * n; ++i) out[i] = 0;
    auto add_at = [&](const uint64_t* arr, size_t len, size_t offset) {
        uint64_t carry = 0;
        for (size_t i = 0; i < len; ++i) {
            unsigned __int128 cur = (unsigned __int128)out[offset+i] + arr[i] + carry;
            out[offset+i] = (uint64_t)cur;
            carry = (uint64_t)(cur >> 64);
        }
        size_t k = offset + len;
        while (carry) {
            unsigned __int128 cur = (unsigned __int128)out[k] + carry;
            out[k] = (uint64_t)cur;
            carry = (uint64_t)(cur >> 64);
            ++k;
        }
    };
    add_at(z0, 2*half, 0);
    add_at(z1, 2*half, half);
    add_at(z2, 2*half, 2*half);
}

// ==========================================
// 3. 任意精度大数核心类
// ==========================================
class dynamic_bigint {
public:
    std::vector<uint64_t> mag;
    bool neg{false};

    dynamic_bigint() = default;
    dynamic_bigint(long long v) {
        if (v < 0) { neg = true; mag.push_back((uint64_t)0 - (uint64_t)v); }
        else if (v > 0) { mag.push_back((uint64_t)v); }
    }
    dynamic_bigint(unsigned long long v) { if (v > 0) mag.push_back(v); }

    bool is_zero() const { return mag.empty(); }
    bool is_negative() const { return neg; }

    void normalize() {
        while (!mag.empty() && mag.back() == 0) mag.pop_back();
        if (mag.empty()) neg = false;
    }

    dynamic_bigint operator-() const {
        dynamic_bigint res = *this;
        if (!res.is_zero()) res.neg = !res.neg;
        return res;
    }

    dynamic_bigint operator+(const dynamic_bigint& o) const { return add_sub(o, false); }
    dynamic_bigint operator-(const dynamic_bigint& o) const { return add_sub(o, true); }

    dynamic_bigint add_sub(const dynamic_bigint& o, bool is_sub) const {
        dynamic_bigint res;
        if (neg == (o.neg ^ is_sub)) {
            size_t n = std::max(mag.size(), o.mag.size());
            res.mag.resize(n, 0);
            uint64_t carry = 0;
            for (size_t i = 0; i < n; ++i) {
                uint64_t a = (i < mag.size()) ? mag[i] : 0;
                uint64_t b = (i < o.mag.size()) ? o.mag[i] : 0;
                uint64_t s = a + b;
                uint64_t c1 = (s < a);
                uint64_t s2 = s + carry;
                uint64_t c2 = (s2 < s);
                res.mag[i] = s2;
                carry = c1 | c2;
            }
            if (carry) res.mag.push_back(carry);
            res.neg = neg;
        } else {
            bool lt = false;
            if (mag.size() != o.mag.size()) lt = mag.size() < o.mag.size();
            else for (int i = (int)mag.size() - 1; i >= 0; --i) if (mag[i] != o.mag[i]) { lt = mag[i] < o.mag[i]; break; }
            
            const auto& a = lt ? o.mag : mag;
            const auto& b = lt ? mag : o.mag;
            res.mag.resize(a.size(), 0);
            uint64_t borrow = 0;
            for (size_t i = 0; i < b.size(); ++i) {
                uint64_t ai = a[i], bi = b[i];
                uint64_t d1 = ai - bi;
                uint64_t b1 = (ai < bi) ? 1 : 0;
                uint64_t d2 = d1 - borrow;
                uint64_t b2 = (d1 < borrow) ? 1 : 0;
                res.mag[i] = d2;
                borrow = b1 | b2;
            }
            for (size_t i = b.size(); borrow && i < a.size(); ++i) {
                uint64_t d = a[i] - borrow;
                borrow = (a[i] < borrow);
                res.mag[i] = d;
            }
            res.neg = lt ? (o.neg ^ is_sub) : neg;
        }
        res.normalize();
        return res;
    }

    dynamic_bigint operator*(const dynamic_bigint& o) const {
        if (is_zero() || o.is_zero()) return dynamic_bigint(0);
        dynamic_bigint res;
        size_t n = std::max(mag.size(), o.mag.size());
        
        if (n >= NTT_THRESHOLD) {
            res.mag = ntt_multiply(mag, o.mag);
        } else if (n >= KARATSUBA_THRESHOLD) {
            res.mag.resize(mag.size() + o.mag.size(), 0);
            std::vector<uint64_t> scratch(n * 8, 0);
            karatsuba_mul(mag.data(), o.mag.data(), res.mag.data(), n, scratch.data());
        } else {
            res.mag.resize(mag.size() + o.mag.size(), 0);
            base_mul(mag.data(), mag.size(), o.mag.data(), o.mag.size(), res.mag.data());
        }
        res.neg = (neg != o.neg);
        res.normalize();
        return res;
    }

    static void divmod(const dynamic_bigint& u_in, const dynamic_bigint& v_in, dynamic_bigint& q, dynamic_bigint& r) {
        if (v_in.is_zero()) throw std::domain_error("divide by zero");
        dynamic_bigint u = u_in.is_negative() ? -u_in : u_in;
        dynamic_bigint v = v_in.is_negative() ? -v_in : v_in;
        if (u.mag.size() < v.mag.size()) { q = dynamic_bigint(0); r = u_in; return; }

        if (v.mag.size() == 1) {
            uint64_t d = v.mag[0];
            q.mag.resize(u.mag.size(), 0);
            uint64_t rem = 0;
            for (size_t i = u.mag.size(); i-- > 0;) {
                U128 qr = udiv128({ rem, u.mag[i] }, d);
                q.mag[i] = qr.hi;
                rem = qr.lo;
            }
            r = dynamic_bigint(rem);
            q.neg = (u_in.neg != v_in.neg) && !q.is_zero();
            r.neg = u_in.neg && !r.is_zero();
            q.normalize(); r.normalize();
            return;
        }

        int shift = clz64(v.mag.back());
        std::vector<uint64_t> un = u.mag, vn = v.mag;
        
        if (shift) {
            uint64_t carry = 0;
            for (size_t i = 0; i < un.size(); ++i) { uint64_t next = un[i] >> (64 - shift); un[i] = (un[i] << shift) | carry; carry = next; }
            if (carry) un.push_back(carry);
            carry = 0;
            for (size_t i = 0; i < vn.size(); ++i) { uint64_t next = vn[i] >> (64 - shift); vn[i] = (vn[i] << shift) | carry; carry = next; }
            if (carry) vn.push_back(carry);
        }
        
        int n = (int)vn.size() - 1;
        int m = (int)un.size() - (int)vn.size();
        q.mag.assign(m + 1, 0);
        un.push_back(0);

        for (int j = m; j >= 0; --j) {
            U128 num = { un[j+n+1], un[j+n] };
            U128 qr = udiv128(num, vn[n]);
            uint64_t qhat = qr.hi;
            uint64_t rhat = qr.lo;
            
            U128 p = mul64x64(qhat, vn[n-1]);
            while (p.hi > rhat || (p.hi == rhat && p.lo > un[j+n-1])) {
                --qhat;
                uint64_t s = rhat + vn[n];
                if (s < rhat) break;
                rhat = s;
                p = mul64x64(qhat, vn[n-1]);
            }
            
            uint64_t borrow = 0, carry = 0;
            for (int i = 0; i <= n; ++i) {
                U128 p2 = mul64x64(qhat, (i < n) ? vn[i] : 0);
                uint64_t lo = p2.lo + carry;
                if (lo < p2.lo) p2.hi++;
                carry = p2.hi;
                uint64_t sub = lo + borrow;
                borrow = (sub < lo) | (un[j + i] < sub);
                un[j + i] -= sub;
            }
            
            if (borrow) {
                --qhat;
                uint64_t c = 0;
                for (int i = 0; i <= n; ++i) {
                    uint64_t s = un[j+i] + ((i < n) ? vn[i] : 0) + c;
                    c = (s < un[j+i]) ? 1 : 0;
                    un[j+i] = s;
                }
            }
            q.mag[j] = qhat;
        }

        if (shift) {
            for (size_t i = 0; i < un.size() - 1; ++i) un[i] = (un[i] >> shift) | (un[i+1] << (64 - shift));
            un.back() >>= shift;
        }
        r.mag.assign(un.begin(), un.begin() + vn.size());
        q.neg = (u_in.neg != v_in.neg) && !q.is_zero();
        r.neg = u_in.neg && !r.is_zero();
        q.normalize(); r.normalize();
    }

    dynamic_bigint operator/(const dynamic_bigint& o) const { dynamic_bigint q, r; divmod(*this, o, q, r); return q; }
    dynamic_bigint operator%(const dynamic_bigint& o) const { dynamic_bigint q, r; divmod(*this, o, q, r); return r; }
    
    dynamic_bigint operator&(const dynamic_bigint& o) const {
        dynamic_bigint res;
        size_t n = std::min(mag.size(), o.mag.size());
        res.mag.resize(n, 0);
        for (size_t i = 0; i < n; ++i) res.mag[i] = mag[i] & o.mag[i];
        res.neg = neg & o.neg;
        res.normalize();
        return res;
    }

    dynamic_bigint operator|(const dynamic_bigint& o) const {
        dynamic_bigint res;
        size_t n = std::max(mag.size(), o.mag.size());
        res.mag.resize(n, 0);
        for (size_t i = 0; i < n; ++i) {
            uint64_t a = (i < mag.size()) ? mag[i] : 0;
            uint64_t b = (i < o.mag.size()) ? o.mag[i] : 0;
            res.mag[i] = a | b;
        }
        res.neg = neg | o.neg;
        res.normalize();
        return res;
    }

    dynamic_bigint operator^(const dynamic_bigint& o) const {
        dynamic_bigint res;
        size_t n = std::max(mag.size(), o.mag.size());
        res.mag.resize(n, 0);
        for (size_t i = 0; i < n; ++i) {
            uint64_t a = (i < mag.size()) ? mag[i] : 0;
            uint64_t b = (i < o.mag.size()) ? o.mag[i] : 0;
            res.mag[i] = a ^ b;
        }
        res.neg = neg ^ o.neg;
        res.normalize();
        return res;
    }

    dynamic_bigint operator>>(size_t k) const {
        if (k == 0) return *this;
        dynamic_bigint res;
        size_t ls = k / 64, bs = k % 64;
        if (ls >= mag.size()) return (neg ? dynamic_bigint(-1) : dynamic_bigint(0));
        res.mag.resize(mag.size() - ls, 0);
        for (size_t i = 0; i < mag.size() - ls; ++i) {
            uint64_t v = mag[i + ls];
            if (bs && i + ls + 1 < mag.size()) v = (v >> bs) | (mag[i + ls + 1] << (64 - bs));
            else if (bs) v >>= bs;
            res.mag[i] = v;
        }
        if (neg && bs && !res.mag.empty()) res.mag.back() |= (~0ULL << (64 - bs));
        res.neg = neg;
        res.normalize();
        return res;
    }

    dynamic_bigint operator<<(size_t k) const {
        if (k == 0 || is_zero()) return *this;
        dynamic_bigint res;
        size_t ls = k / 64, bs = k % 64;
        res.mag.resize(mag.size() + ls + (bs ? 1 : 0), 0);
        for (size_t i = 0; i < mag.size(); ++i) {
            res.mag[i + ls] |= mag[i] << bs;
            if (bs) res.mag[i + ls + 1] |= mag[i] >> (64 - bs);
        }
        res.neg = neg;
        res.normalize();
        return res;
    }

    bool operator<(const dynamic_bigint& o) const {
        if (neg != o.neg) return neg;
        if (mag.size() != o.mag.size()) {
            return neg ? (mag.size() > o.mag.size()) : (mag.size() < o.mag.size());
        }
        for (int i = (int)mag.size() - 1; i >= 0; --i) {
            if (mag[i] != o.mag[i]) return neg ? (mag[i] > o.mag[i]) : (mag[i] < o.mag[i]);
        }
        return false;
    }
    
    bool operator<=(const dynamic_bigint& o) const { return !(o < *this); }
    bool operator>(const dynamic_bigint& o) const { return o < *this; }
    bool operator>=(const dynamic_bigint& o) const { return !(*this < o); }

    bool operator==(const dynamic_bigint& o) const { return neg == o.neg && mag == o.mag; }
    bool operator!=(const dynamic_bigint& o) const { return !(*this == o); }

    std::string to_string() const {
        if (is_zero()) return "0";
        std::string out;
        dynamic_bigint tmp = neg ? -(*this) : *this;
        dynamic_bigint base(10000000000000000000ULL);
        std::vector<std::string> chunks;
        
        while (!tmp.is_zero()) {
            dynamic_bigint q, r;
            divmod(tmp, base, q, r);
            uint64_t rem = r.is_zero() ? 0 : r.mag[0];
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%019llu", (unsigned long long)rem);
            chunks.push_back(buf);
            tmp = q;
        }
        
        std::string str_out;
        if (neg) str_out.push_back('-');
        std::string highest = chunks.back();
        size_t start = highest.find_first_not_of('0');
        if (start != std::string::npos) str_out += highest.substr(start);
        else str_out += "0";
        
        for (int i = (int)chunks.size() - 2; i >= 0; --i) str_out += chunks[i];
        return str_out;
    }

    std::string to_hex() const {
        if (is_zero()) return "0";
        std::string out;
        const char* hex = "0123456789abcdef";
        bool started = false;
        for (int i = (int)mag.size() - 1; i >= 0; --i) {
            uint64_t v = mag[i];
            for (int j = 15; j >= 0; --j) {
                int d = (v >> (j * 4)) & 0xF;
                if (d != 0 || started) { started = true; out.push_back(hex[d]); }
            }
        }
        return out.empty() ? "0" : (neg ? "-" : "") + out;
    }

    friend std::ostream& operator<<(std::ostream& os, const dynamic_bigint& v) { return os << v.to_string(); }
};

// ==========================================
// 4. 数论与数学算法全家桶
// ==========================================
inline dynamic_bigint abs(dynamic_bigint x) { return x.is_negative() ? -x : x; }

inline dynamic_bigint gcd(dynamic_bigint a, dynamic_bigint b) {
    a = abs(a); b = abs(b);
    while (!b.is_zero()) { dynamic_bigint t = b; b = a % b; a = t; }
    return a;
}

inline dynamic_bigint lcm(dynamic_bigint a, dynamic_bigint b) {
    if (a.is_zero() || b.is_zero()) return dynamic_bigint(0);
    return abs((a / gcd(a, b)) * b);
}

inline dynamic_bigint pow(dynamic_bigint base, dynamic_bigint exp) {
    if (exp.is_negative()) throw std::invalid_argument("pow: negative exponent");
    dynamic_bigint res(1);
    while (!exp.is_zero()) {
        if ((exp & dynamic_bigint(1)) == dynamic_bigint(1)) res = res * base;
        base = base * base; exp = exp >> 1;
    }
    return res;
}

inline dynamic_bigint mul_mod(const dynamic_bigint& a, const dynamic_bigint& b, const dynamic_bigint& mod) { return (a * b) % mod; }
inline dynamic_bigint pow_mod(dynamic_bigint base, dynamic_bigint exp, const dynamic_bigint& mod) {
    if (exp.is_negative()) throw std::invalid_argument("pow_mod: negative exponent");
    dynamic_bigint res(1); base = base % mod;
    if (base.is_negative()) base = base + mod;
    while (!exp.is_zero()) {
        if ((exp & dynamic_bigint(1)) == dynamic_bigint(1)) res = mul_mod(res, base, mod);
        base = mul_mod(base, base, mod); exp = exp >> 1;
    }
    return res;
}

inline bool is_prime(const dynamic_bigint& n) {
    if (n < dynamic_bigint(2)) return false;
    if (n == dynamic_bigint(2) || n == dynamic_bigint(3)) return true;
    if ((n & dynamic_bigint(1)).is_zero()) return false;

    dynamic_bigint d = n - dynamic_bigint(1); int s = 0;
    while ((d & dynamic_bigint(1)).is_zero()) { d = d >> 1; s++; }

    std::vector<unsigned long long> bases = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37};
    for (unsigned long long a : bases) {
        dynamic_bigint a_big(a);
        if (a_big >= n) break;
        dynamic_bigint x = pow_mod(a_big, d, n);
        if (x == dynamic_bigint(1) || x == n - dynamic_bigint(1)) continue;

        bool composite = true;
        for (int r = 1; r < s; ++r) {
            x = mul_mod(x, x, n);
            if (x == n - dynamic_bigint(1)) { composite = false; break; }
        }
        if (composite) return false;
    }
    return true;
}

inline dynamic_bigint pollard_rho(const dynamic_bigint& n) {
    if (n == dynamic_bigint(2)) return 2;
    if (n == dynamic_bigint(3)) return 3;
    if ((n & dynamic_bigint(1)).is_zero()) return 2;

    std::random_device rd;
    std::mt19937_64 rng(rd());
    for (int retry = 0; retry < 100; ++retry) {
        dynamic_bigint c = dynamic_bigint(rng() % 100) + dynamic_bigint(1);
        dynamic_bigint x = dynamic_bigint(rng() % 100) + dynamic_bigint(2);
        dynamic_bigint y = x;
        dynamic_bigint d = dynamic_bigint(1);
        for (int steps = 0; steps < (1 << 20); ++steps) {
            x = (mul_mod(x, x, n) + c) % n;
            y = (mul_mod(y, y, n) + c) % n;
            y = (mul_mod(y, y, n) + c) % n;
            d = gcd(abs(x - y), n);
            if (d != dynamic_bigint(1)) break;
        }
        if (d != n && d != dynamic_bigint(1)) return d;
    }
    throw std::runtime_error("pollard_rho: failed to find factor");
}

inline void factorize_recursive(dynamic_bigint n, std::vector<dynamic_bigint>& factors) {
    if (n == dynamic_bigint(1)) return;
    if (is_prime(n)) { factors.push_back(n); return; }
    dynamic_bigint d = pollard_rho(n);
    factorize_recursive(d, factors);
    factorize_recursive(n / d, factors);
}

inline std::vector<dynamic_bigint> factorize(dynamic_bigint n) {
    if (n.is_negative()) n = -n;
    std::vector<dynamic_bigint> factors;
    if (n.is_zero() || n == dynamic_bigint(1)) return factors;
    for (unsigned long long p : {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37}) {
        dynamic_bigint p_big(p);
        while ((n % p_big).is_zero()) { factors.push_back(p_big); n = n / p_big; }
    }
    if (n > dynamic_bigint(1)) factorize_recursive(n, factors);
    std::sort(factors.begin(), factors.end());
    return factors;
}

inline dynamic_bigint mod_inverse(dynamic_bigint a, dynamic_bigint m) {
    if (m <= dynamic_bigint(0)) throw std::invalid_argument("mod_inverse: modulus must be positive");
    if (m == dynamic_bigint(1)) return dynamic_bigint(0);
    a = (a % m + m) % m;
    if (a.is_zero()) throw std::domain_error("mod_inverse: no inverse for 0");
    dynamic_bigint m0 = m;
    dynamic_bigint x0 = dynamic_bigint(0), x1 = dynamic_bigint(1);
    while (a > dynamic_bigint(1)) {
        dynamic_bigint q = a / m;
        dynamic_bigint t = m;
        m = a % m; a = t;
        t = x0; x0 = x1 - q * x0; x1 = t;
    }
    if (x1 < dynamic_bigint(0)) x1 = x1 + m0;
    return x1;
}

} // namespace fbig
#endif // FBIG_HPP
