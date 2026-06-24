#include "toyc/backend/optimizer.hpp"
#include "toyc/backend/riscv32_codegen.hpp"
#include "toyc/backend/text_ir.hpp"

#include <exception>
#include <iostream>
#include <string>

using namespace toyc::backend;

namespace {

void printUsage(const char* executable) {
    std::cerr << "usage: " << executable << " [-opt] [--no-comments]\n"
              << "Reads textual backend IR from stdin and writes RV32IM "
                 "assembly to stdout.\n";
}

} // namespace

int main(int argc, char** argv) {
    bool enable_optimization = false;
    CodegenOptions codegen_options;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "-opt") {
            enable_optimization = true;
        } else if (argument == "--no-comments") {
            codegen_options.emit_comments = false;
        } else if (argument == "-h" || argument == "--help") {
            printUsage(argv[0]);
            return 0;
        } else {
            printUsage(argv[0]);
            return 2;
        }
    }

    auto parsed = parseTextIr(std::cin);
    if (!parsed.ok()) {
        for (const auto& error : parsed.errors) {
            std::cerr << "error: " << error << '\n';
        }
        return 1;
    }

    try {
        auto program = enable_optimization
                           ? optimize(std::move(parsed.program))
                           : std::move(parsed.program);
        std::cout << RiscV32CodeGenerator{codegen_options}.generate(program);
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
