#include "bigint.hpp"
#include <iostream>
#include <cstdlib>

static void run_tests() {
    using B = bigint;
    int passed = 0, failed = 0;
    
    auto check = [&](bool cond, const char* name) {
        if (cond) passed++;
        else {
            std::cout << "[FAIL] " << name << "\n";
            failed++;
        }
    };

    check(B::from_string("123") + B::from_string("456") == B::from_string("579"), "Add basic");
    check(B::from_string("-100") * B::from_string("3") == B::from_string("-300"), "Mul negative");
    check(B::from_string("100") / B::from_string("7") == B::from_string("14"), "Div basic");
    check(B::from_string("100") % B::from_string("7") == B::from_string("2"), "Mod basic");
    check(B::from_hex("0xFF") == B(255), "Hex parse");
    check(B::from_oct_str("0o55") == B(45), "Octal parse");
    
    check(fact(5) == B(120), "Fact 5");
    check(pow(B(2), 10) == B(1024), "Pow 2^10");
    check(isqrt(B(144)) == B(12), "Isqrt 144");
    check(gcd(B(54), B(24)) == B(6), "GCD 54,24");
    check(pow_mod(B(2), B(10), B(1000)) == B(24), "Pow_mod 2^10 % 1000");

    B x = (B(1) << 4096) - B(1);
    B expect = (B(1) << 8192) - (B(1) << 4097) + B(1);
    check(x * x == expect, "Karatsuba square 4096-bit");

    std::cout << "Tests passed: " << passed << ", failed: " << failed << "\n";
    if (failed > 0) std::exit(1);
}

int main() {
    run_tests();

    std::cout << "\n--- bigint v0.4.1 Demo ---\n";
    std::cout << "fact(20) = " << fact(20) << "\n";
    std::cout << "pow(2, 100) = " << pow(2, 100) << "\n";
    std::cout << "isqrt(10^100) = " << isqrt(pow(10, 100)) << "\n";
    std::cout << "gcd(1071, 462) = " << gcd(1071, 462) << "\n";
    std::cout << "pow_mod(2, 100, 1000000007) = " << pow_mod(2, 100, 1000000007) << "\n";

    return 0;
}
