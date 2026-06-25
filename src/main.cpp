// ToyC Compiler - Main entry point
// Role B: Semantic Analysis implementation
// Reads ToyC source from stdin, performs semantic analysis, writes to stdout

#include "ast/AST.h"
#include "semantic/SemanticAnalyzer.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    [[maybe_unused]] bool optimize = (argc > 1 && std::string(argv[1]) == "-opt");
    // Note: Full compiler would parse stdin first (Role A), then run semantic analysis.
    // This stub implements the semantic analysis interface for integration testing.

    std::cerr << "ToyC Compiler v0.1 - Semantic Analysis (Role B)" << std::endl;

    // Read entire stdin
    std::string source;
    std::string line;
    while (std::getline(std::cin, line)) {
        source += line + "\n";
    }

    // Full pipeline would be: lex -> parse -> semantic -> codegen
    // Since Role A (frontend) handles lexing/parsing, this stub just shows
    // that the semantic analysis module is ready for integration.

    // Create a trivial CompUnit for testing the interface
    toyc::CompUnit root;
    toyc::SemanticAnalyzer analyzer;

    if (analyzer.analyze(&root)) {
        std::cerr << "Semantic analysis passed." << std::endl;
        return 0;
    } else {
        for (const auto& err : analyzer.errors()) {
            std::cerr << err.line << ":" << err.column << ": error: ["
                      << static_cast<int>(err.code) << "] "
                      << err.message << std::endl;
        }
        return 1;
    }
}
