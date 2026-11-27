#include "fbig.hpp"
#include <iostream>
#include <chrono>

using namespace fbig;

int main() {
    std::cout << "===== fbig 任意精度完全体测试 =====" << std::endl;

    // 1. 测试数论
    dynamic_bigint n = 1234567890;
    std::cout << "is_prime(1234567890)? " << (is_prime(n) ? "Yes" : "No") << std::endl;
    
    dynamic_bigint p = 1000000007, q = 1000000009;
    dynamic_bigint composite = p * q;
    std::cout << "p * q = " << composite << std::endl;
    
    auto factors = factorize(composite);
    std::cout << "factors of p*q: ";
    for (auto& f : factors) std::cout << f << " ";
    std::cout << std::endl;

    // 2. 测试模逆元 (3 * 5 = 15 ≡ 1 mod 7)
    std::cout << "3^-1 mod 7 = " << mod_inverse(3, 7) << std::endl;

    // 3. 测试大数 NTT 乘法
    dynamic_bigint a(1);
    a = a << 40000; // 2^40000
    dynamic_bigint b = a - 1;
    auto start = std::chrono::high_resolution_clock::now();
    dynamic_bigint c = a * b;
    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "2^40000 * (2^40000 - 1) 结果十六进制长度: " << c.to_hex().length() << ", 耗时: " << ms << " ms" << std::endl;

    return 0;
}
