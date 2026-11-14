#ifndef BIGINT_HPP
#define BIGINT_HPP

#include <iostream>
#include <vector>
#include <string>
#include <span>
#include <cstdint>

class bigint {
public:
    using limb_t = uint64_t;
    using mag_t  = std::vector<limb_t>;
    using view_t = std::span<const limb_t>;

    static constexpr size_t KARATSUBA_THRESHOLD  = 32;
    static constexpr size_t SMALL_LIMB_THRESHOLD = 4;
    static constexpr limb_t BASE10_19 = 10'000'000'000'000'000'000ULL;

private:
    bool  neg{false};
    mag_t mag;

    void normalize();
    static view_t trimmed(view_t v);
    static bool mag_lt(view_t a, view_t b);
    static bool mag_lt(const mag_t& a, const mag_t& b);
    static void mag_add_view(view_t a, view_t b, mag_t& out);
    static void mag_add(mag_t& a, const mag_t& b);
    static void mag_sub_view(mag_t& a, view_t b);
    static void mag_sub(mag_t& a, const mag_t& b);
    static void mag_mul_limb_into(view_t a, limb_t b, mag_t& out);
    static void mag_mul_into_4way(view_t a, view_t b, mag_t& out);
    static void add_at(mag_t& out, const mag_t& z, size_t off);
    static void mag_mul_karatsuba_v(view_t a, view_t b, mag_t& out, struct Scratch& sc);
    static void mag_sqr_into(view_t a, mag_t& out);
    static void mag_shl(mag_t& a, size_t k);
    static void mag_shr(mag_t& a, size_t k);
    static void mag_divmod(view_t u_in, view_t v_in, mag_t& q, mag_t& r);
    static mag_t to_twos(const bigint& x, size_t W);
    static bigint from_twos(mag_t m);
    static bigint from_dec_str(const std::string& s);

public:
    bigint() = default;
    bigint(long long v);
    bigint(const bigint&) = default;
    bigint& operator=(const bigint&) = default;
    bigint(bigint&&) noexcept = default;
    bigint& operator=(bigint&&) noexcept = default;

    void swap(bigint& o) noexcept;
    bool is_zero() const noexcept;
    bool is_odd() const noexcept;

    static bigint from_hex(const std::string& s);
    static bigint from_oct_str(const std::string& s);
    static bigint from_string(const std::string& s);

    friend std::istream& operator>>(std::istream& is, bigint& v);
    friend std::ostream& operator<<(std::ostream& os, const bigint& v);

    bigint operator-() const;
    bigint operator~() const;
    bigint operator+(const bigint& o) const;
    bigint operator-(const bigint& o) const;
    bigint operator*(const bigint& o) const;
    bigint operator/(const bigint& o) const;
    bigint operator%(const bigint& o) const;
    
    bigint sqr() const;
    bigint pow(const bigint& exp) const;
    bigint fact() const;

    static void divmod(const bigint& x, const bigint& y, bigint& q, bigint& r);

    bigint operator<<(size_t k) const;
    bigint operator>>(size_t k) const;
    bigint operator&(const bigint& o) const;
    bigint operator|(const bigint& o) const;
    bigint operator^(const bigint& o) const;

    bigint& operator+=(const bigint& o);
    bigint& operator-=(const bigint& o);
    bigint& operator*=(const bigint& o);
    bigint& operator/=(const bigint& o);
    bigint& operator%=(const bigint& o);
    bigint& operator&=(const bigint& o);
    bigint& operator|=(const bigint& o);
    bigint& operator^=(const bigint& o);
    bigint& operator<<=(size_t k);
    bigint& operator>>=(size_t k);

    bigint& operator++();
    bigint  operator++(int);
    bigint& operator--();
    bigint  operator--(int);

    bool operator==(const bigint& o) const;
    bool operator!=(const bigint& o) const;
    bool operator<(const bigint& o) const;
    bool operator<=(const bigint& o) const;
    bool operator>(const bigint& o) const;
    bool operator>=(const bigint& o) const;

    bool fits_ll() const;
    long long to_ll() const;
    std::string to_string() const;
    std::string to_hex() const;
};

// 数学函数扩展
inline bigint pow(const bigint& base, const bigint& exp) { return base.pow(exp); }
inline bigint fact(const bigint& n) { return n.fact(); }
inline bigint isqrt(const bigint& n);
inline bigint gcd(const bigint& a, const bigint& b);
inline bigint pow_mod(bigint base, bigint exp, const bigint& mod);

#endif // BIGINT_HPP
