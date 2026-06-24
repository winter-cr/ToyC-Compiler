#pragma once

#include "toyc/backend/ir.hpp"

#include <iosfwd>
#include <string>
#include <vector>

namespace toyc::backend {

struct ParseResult {
    Program program;
    std::vector<std::string> errors;

    [[nodiscard]] bool ok() const { return errors.empty(); }
};

// Parses the line-oriented backend IR documented in backend/README.md.
[[nodiscard]] ParseResult parseTextIr(std::istream& input);

} // namespace toyc::backend
