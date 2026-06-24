#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#include "ast/AstPrinter.h"
#include "lexer/LexError.h"
#include "lexer/Lexer.h"
#include "parser/Parser.h"

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
    (void)optimize;

    try {
        const std::string source = read_all_stdin();
        Lexer lexer(source);
        Parser parser(lexer.tokens());
        std::unique_ptr<CompUnit> ast = parser.parse();

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

        // 语义分析与代码生成由后续模块负责
        std::cout << "// ToyC frontend: parse succeeded\n";
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
