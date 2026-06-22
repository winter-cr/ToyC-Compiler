#pragma once

#include "toyc/backend/ir.hpp"

#include <string>

namespace toyc::backend {

struct CodegenOptions {
    bool emit_comments{true};
};

class RiscV32CodeGenerator {
public:
    explicit RiscV32CodeGenerator(CodegenOptions options = {});

    [[nodiscard]] std::string generate(const Program& program) const;

private:
    CodegenOptions options_;
};

} // namespace toyc::backend
