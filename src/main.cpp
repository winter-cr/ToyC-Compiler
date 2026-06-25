#include "ast/AstPrinter.h"
#include "codegen/AstToIr.h"
#include "lexer/LexError.h"
#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "semantic/SemanticAnalyzer.h"
#include "toyc/backend/optimizer.hpp"
#include "toyc/backend/riscv32_codegen.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

namespace {

std::string read_all_stdin() {
    std::ostringstream buffer;
    buffer << std::cin.rdbuf();
    return buffer.str();
}

bool env_enabled(const char* name) {
#if defined(_MSC_VER)
    char* value = nullptr;
    size_t len = 0;
    if (_dupenv_s(&value, &len, name) != 0 || value == nullptr) {
        return false;
    }
    const bool enabled = value[0] != '\0' && value[0] != '0';
    free(value);
    return enabled;
#else
    const char* value = std::getenv(name);
    return value != nullptr && value[0] != '\0' && value[0] != '0';
#endif
}

} // namespace

int main(int argc, char* argv[]) {
    bool optimize = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-opt") {
            optimize = true;
        } else {
            std::cerr << "unknown argument: " << argv[i] << '\n';
            return 1;
        }
    }

    try {
        // 1. Lex + Parse
        const std::string source = read_all_stdin();
        Lexer lexer(source);
        Parser parser(lexer.tokens());
        std::unique_ptr<toyc::CompUnit> ast = parser.parse();

        bool failed = false;
        for (const LexError& error : lexer.errors()) {
            std::cerr << "lexical error: " << error.what() << '\n';
            failed = true;
        }
        for (const ParseError& error : parser.errors()) {
            std::cerr << "parse error: " << error.what() << '\n';
            failed = true;
        }
        if (failed) {
            return 1;
        }

        if (env_enabled("TOYC_DUMP_AST")) {
            AstPrinter printer(std::cerr);
            printer.print(*ast);
        }

        // 2. Semantic analysis
        toyc::SemanticAnalyzer analyzer;
        if (!analyzer.analyze(ast.get())) {
            for (const auto& err : analyzer.errors()) {
                std::cerr << err.line << ":" << err.column << ": error: ["
                          << static_cast<int>(err.code) << "] "
                          << err.message << '\n';
            }
            return 1;
        }

        // 3. AST → IR
        toyc::AstToIr ast2ir;
        auto program = ast2ir.convert(ast.get());

        // 4. Optional optimization
        if (optimize) {
            program = toyc::backend::optimize(std::move(program));
        }

        // 5. Code generation
        toyc::backend::CodegenOptions options;
        options.emit_comments = env_enabled("TOYC_EMIT_COMMENTS");
        std::cout << toyc::backend::RiscV32CodeGenerator{options}.generate(program);

    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
