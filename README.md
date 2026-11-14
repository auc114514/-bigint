格式基于 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.0.0/)，
并且本项目遵循 [语义化版本 (Semantic Versioning)](https://semver.org/lang/zh-CN/)。
## [0.4.1] - 2026-10-14

### 修复 (Fixed)
- **修复了严重编译错误**：解决了将单文件拆分为 `.hpp` / `.cpp` 时，内部类 `Scratch` 在头文件中仅有前向声明导致的外部无法实例化问题。将其完整定义移至 `bigint.hpp` 的私有区域。
- **修复了头文件缺失依赖**：在 `bigint.hpp` 中补充了缺失的 `<deque>` 头文件。
- **修复了链接属性错误**：移除了头文件中数学扩展函数（`isqrt`, `gcd`, `pow_mod`）声明前的 `inline` 关键字，避免了分离式编译时的链接器警告和误导。

### 变更 (Changed)
- 优化了测试用例结构：新增了独立编译的 `test_bigint.cpp`，以确保在 CI 环境下能够正确分离编译和链接，验证库的真实可用性。

## [0.4.0] - 2026-10-14 (Current)

### 新增 (Added)
- 新增 `gcd()` 函数，支持大数最大公约数计算。
- 新增 `pow_mod()` 函数，实现基于快速幂的大数取模运算。
- 新增 `isqrt()` 函数，基于 Newton 迭代法实现整数平方根。
- 新增轻量级测试框架（替换了纯 `assert`），支持统计 PASS/FAIL 并在失败时返回非零退出码。
- 增加 `operator+=` 的原地修改（In-place）优化，减少大数加法时的临时对象拷贝。

### 变更 (Changed)
- **工程化重构**：移除了 `<bits/stdc++.h>` 和 `using namespace std;`，改用标准头文件并显式使用 `std::` 前缀，避免污染用户命名空间。
- 将全局求平方根函数 `sqrt` 重命名为 `isqrt`，以避免与 `<cmath>` 中的 `std::sqrt` 产生二义性冲突。
- 将单文件构建改造为标准头文件/源文件分离结构（*若有拆分为 .hpp/.cpp 的话*）。

### 修复 (Fixed)
- 修复了 `fact()` 函数在传入超出 `long long` 范围的大数时可能导致的静默截断与死循环问题，现在会抛出 `std::overflow_error`。
- 在 `add_at` 和 `mag_sqr_into` 的进位循环中增加了越界断言（`assert`），提升了 Debug 模式下的安全性。

### 移除 (Removed)
- 移除了不必要的死代码（未公开暴露的定宽位运算相关辅助函数）。

---

## [0.3.0] - 2026-10-14

### 新增 (Added)
- 添加 `sqr()` 专用接口，优化大数平方计算性能。
- 添加 `pow()` 核心快速幂实现。

### 变更 (Changed)
- **语法糖移除**：废弃并移除了极易引发 C++ 运算符优先级陷阱和编译报错的 `_p` 和 `pow_tag` 等自定义字面量/运算符重载，改用标准的 `pow(base, exp)` 函数调用形式。

### 修复 (Fixed)
- 修复了 `mag_add` 在目标与源同址（Aliasing）时因 `assign` 导致的数据自毁问题（改用临时变量中转）。
- 修复了 `Scratch` 内存池使用 `std::vector` 作为底层容器时，扩容导致外层引用悬空（Dangling Reference）的致命 UB 问题，改用 `std::deque`。
- 修复了 `to_string()` 中因为 `10^19` 被强转为 `long long` 导致回绕，进而使大数十进制输出全盘错误的严重 Bug。

---

## [0.2.0] - 历史版本

### 新增 (Added)
- 引入 C++20 `std::span`，实现 Karatsuba 乘法过程中的零拷贝（Zero-copy）视图切分。
- 引入 `Scratch` 临时内存池，减少深度递归乘法中的动态内存分配开销。
- 实现 4 路累加器（4-way accumulator）优化的 O(n²) 乘法内核，提升指令级并行（ILP）。
- 支持 `0x` 和 `0o` 前缀的进制字符串解析。

### 变更 (Changed)
- 乘法策略分为四级分派：单 limb 快速路径、短操作数直乘、O(n²) 4路累加、Karatsuba。
- 优化 `from_dec_str`，采用每 19 位十进制分批转换，大幅减少除法次数。

---

## [0.1.0] - 初始原型

### 新增 (Added)
- 从定宽 `int1024_t` 重构为任意精度 `bigint`。
- 采用符号-幅值（Sign-Magnitude）表示法，底层使用 `std::vector<uint64_t>` 存储。
- 实现了完整的基础四则运算（加减乘除）、取模运算。
- 实现了基于 Knuth Algorithm D 的大数除法/取模。
- 支持负数及算术右移（补符号位）语义。

---

## [0.0.0] - 开始

### 创作
- 创作了 `int1024_t`。
- 增加了一些基础运算。
- 实现了位运算（与、或、异或、取反）及流输入输出（`<<` / `>>`）。

---

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/).
## [0.4.1] - 2026-10-14

### Fixed
- **Fixed a serious compilation error**: Resolved an issue where, when splitting the single file into `.hpp` / `.cpp`, the internal class `Scratch` was only forward-declared in the header, making it impossible to instantiate externally. Moved its complete definition into the private section of `bigint.hpp`.
- **Fixed missing header dependency**: Added the missing `<deque>` header to `bigint.hpp`.
- **Fixed incorrect linkage attribute**: Removed the `inline` keyword before the declarations of math extension functions (`isqrt`, `gcd`, `pow_mod`) in the header, avoiding linker warnings and misleading behavior during separate compilation.

### Changed
- Optimized the test case structure: Added a separately compiled `test_bigint.cpp` to ensure that separate compilation and linking can be correctly performed in a CI environment, verifying the real usability of the library.

## [0.4.0] - 2026-10-14 (Current)

### Added
- Added the `gcd()` function, supporting greatest common divisor computation for big numbers.
- Added the `pow_mod()` function, implementing big number modular exponentiation based on fast exponentiation.
- Added the `isqrt()` function, implementing integer square root based on Newton's iteration method.
- Added a lightweight testing framework (replacing plain `assert`), supporting PASS/FAIL statistics and returning a non-zero exit code on failure.
- Added an in-place optimization for `operator+=`, reducing temporary object copies during big number addition.

### Changed
- **Engineering refactor**: Removed `<bits/stdc++.h>` and `using namespace std;`, switched to standard headers and explicitly used the `std::` prefix to avoid polluting the user's namespace.
- Renamed the global square root function `sqrt` to `isqrt` to avoid ambiguity conflicts with `std::sqrt` in `<cmath>`.
- Refactored the single-file build into a standard header/source file separation structure (*if split into .hpp/.cpp*).

### Fixed
- Fixed a potential silent truncation and infinite loop issue in the `fact()` function when passed a big number exceeding the `long long` range; it now throws `std::overflow_error`.
- Added out-of-bounds assertions (`assert`) in the carry loops of `add_at` and `mag_sqr_into`, improving safety in Debug mode.

### Removed
- Removed unnecessary dead code (non-publicly-exposed fixed-width bitwise operation helper functions).

---

## [0.3.0] - 2026-10-14

### Added
- Added a dedicated `sqr()` interface to optimize big number squaring performance.
- Added the core fast exponentiation implementation of `pow()`.

### Changed
- **Syntactic sugar removal**: Deprecated and removed custom literals/operator overloads such as `_p` and `pow_tag`, which easily caused C++ operator precedence pitfalls and compilation errors, and switched to the standard `pow(base, exp)` function call form.

### Fixed
- Fixed a data self-destruction issue in `mag_add` caused by `assign` when the target and source are at the same address (aliasing) (switched to using a temporary variable as an intermediary).
- Fixed a fatal UB issue where the `Scratch` memory pool used `std::vector` as its underlying container, and reallocation caused outer references to dangle (dangling reference); switched to `std::deque`.
- Fixed a serious bug in `to_string()` where `10^19` was cast to `long long`, causing wraparound and thereby making big number decimal output entirely incorrect.

---

## [0.2.0] - Historical version

### Added
- Introduced C++20 `std::span` to implement zero-copy view splitting during Karatsuba multiplication.
- Introduced the `Scratch` temporary memory pool to reduce dynamic memory allocation overhead in deeply recursive multiplication.
- Implemented an O(n²) multiplication kernel optimized with a 4-way accumulator, improving instruction-level parallelism (ILP).
- Supported parsing of base-prefixed strings with `0x` and `0o`.

### Changed
- Multiplication strategy is divided into four-level dispatch: single-limb fast path, direct multiplication for short operands, O(n²) 4-way accumulation, and Karatsuba.
- Optimized `from_dec_str` by converting decimal digits in batches of 19 at a time, greatly reducing the number of divisions.

---

## [0.1.0] - Initial prototype

### Added
- Refactored from fixed-width `int1024_t` to arbitrary-precision `bigint`.
- Adopted sign-magnitude representation, with `std::vector<uint64_t>` as the underlying storage.
- Implemented complete basic four arithmetic operations (addition, subtraction, multiplication, division) and modulo operation.
- Implemented big number division/modulo based on Knuth Algorithm D.
- Supported negative numbers and arithmetic right shift (sign-bit extension) semantics.

---

## [0.0.0] - Beginning

### Creation
- Created `int1024_t`.
- Added some basic operations.
- Implemented bitwise operations (AND, OR, XOR, NOT) and stream input/output (`<<` / `>>`).
