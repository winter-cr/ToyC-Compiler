#pragma once

#include "toyc/backend/ir.hpp"

namespace toyc::backend {

struct OptimizationOptions {
    bool fold_constants{true};
    bool simplify_branches{true};
    bool eliminate_dead_temporaries{true};
};

// Runs conservative, semantics-preserving local optimizations. Mutable locals
// and globals are never removed by the dead-temporary pass.
[[nodiscard]] Program optimize(
    Program program, OptimizationOptions options = {});

} // namespace toyc::backend
