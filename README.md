我将手搓一个 `Bigint` 类型！

最新版本如下：
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


你可以在 QQ 的 `3801852720` 找到我

---

I'm going to hand-roll a `Bigint` type!

The latest version is as follows:
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


You can find me on QQ at `3801852720`
