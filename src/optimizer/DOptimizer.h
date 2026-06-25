#pragma once

#include "toyc/backend/ir.hpp"

#include <string>

namespace toyc {

// D 成员的优化模块：IR 级跳转合并 + 冗余消除 + 汇编级 peephole。
// 与 C 的 backend::optimize() 互补：C 做常量折叠/分支简化/死 temporaries，
// D 做控制流简化和指令级优化。

class DOptimizer {
public:
    // IR-level optimizations.
    toyc::backend::Program optimizeIR(toyc::backend::Program program);

    // Assembly-level peephole pass. Runs after code generation.
    std::string optimizeAssembly(const std::string& assembly);
};

} // namespace toyc
