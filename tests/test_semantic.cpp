#include "ast/AST.h"
#include "semantic/SemanticAnalyzer.h"
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace toyc;

int main() {
    int passed = 0, total = 0;
    auto check = [&](const char* name, bool ok) {
        total++;
        std::cout << (ok ? "[PASS]" : "[FAIL]") << " " << name << "\n";
        if (ok) passed++;
        };

    // Test 1: int main(void) return 0; }
    {
        auto body = std::make_unique<Block>();
        body->statements.push_back(
            std::make_unique<ReturnStmt>(std::make_unique<Number>(0)));
        std::vector<std::unique_ptr<Param>> params;
        auto func = std::make_unique<FuncDef>(
            Type::intType(), "main", std::move(params), std::move(body));
        auto cu = std::make_unique<CompUnit>();
        cu->elements.push_back(std::move(func));
        SemanticAnalyzer a;
        check("Test 1: valid empty main",
            a.analyze(cu.get()) && !a.hasErrors());
    }

    // Test 2: void main(void) {}
    {
        std::vector<std::unique_ptr<Param>> params;
        auto func = std::make_unique<FuncDef>(
            Type::voidType(), "main", std::move(params),
            std::make_unique<Block>());
        auto cu = std::make_unique<CompUnit>();
        cu->elements.push_back(std::move(func));
        SemanticAnalyzer a;
        check("Test 2: void main rejected", !a.analyze(cu.get()));
    }

    // Test 3: const N = 42, check getConstantValue
    {
        auto cu = std::make_unique<CompUnit>();
        cu->elements.push_back(
            std::make_unique<ConstDecl>("N",
                std::make_unique<Number>(42), true));
        auto body = std::make_unique<Block>();
        body->statements.push_back(
            std::make_unique<ReturnStmt>(std::make_unique<Number>(0)));
            std::vector<std::unique_ptr<Param>> params;
            cu->elements.push_back(std::make_unique<FuncDef>(
                Type::intType(), "main", std::move(params), std::move(body)));
            SemanticAnalyzer a;
            bool ok = a.analyze(cu.get());
            int val = 0;
            bool found = a.getConstantValue("N", &val);
            check("Test 3: const eval + getConstantValue",
                ok && found && val == 42);
    }

    // Test 4: duplicate globals
    {
        auto cu = std::make_unique<CompUnit>();
        cu->elements.push_back(
            std::make_unique<VarDecl>("x",
                std::make_unique<Number>(1), true));
        cu->elements.push_back(
            std::make_unique<VarDecl>("x",
                std::make_unique<Number>(2), true));
        auto body = std::make_unique<Block>();
        body->statements.push_back(
            std::make_unique<ReturnStmt>(std::make_unique<Number>(0)));
        std::vector<std::unique_ptr<Param>> params;
        cu->elements.push_back(std::make_unique<FuncDef>(
            Type::intType(), "main", std::move(params), std::move(body)));
        SemanticAnalyzer a;
        check("Test 4: duplicate global rejected",
            !a.analyze(cu.get()));
    }

    std::cout << "\n=== " << passed << "/" << total << " passed ===\n";
    return (passed == total) ? 0 : 1;
}
