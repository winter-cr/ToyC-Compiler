import os

# Generate the test_semantic.cpp file
content = r'''#include "ast/AST.h"
#include "semantic/SemanticAnalyzer.h"
#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <memory>
#includetype_traits>

using namespace toyc;
using namespace std;

// =====================================================================
// Helper functions build ASTs manually// =====================================================================

unique_ptr<Number> makeNumber(int val) {
    return make_unique<Numberval);
}

unique_ptr<Identifier> make(string name) {
    return make_unique<Identifier>(move));
}

unique_ptr<BinaryExpr> makeBin(unique_ptr<Expr> l, BinaryExpr::Op op,
                                unique_ptr<Expr> r) {
    return make_unique<BinaryExpr>(move(l), op, move(r));
}

unique_ptr<FuncCall> makeCall(string name, vector<unique_ptr<Expr>> args) {
    return make_unique<FuncCall>(move(name), move(args));
}

unique_ptr<ReturnStmt> makeReturn(unique_ptr<Expr> val = nullptr) {
    return make_unique<ReturnStmt>(move(val));
}

unique_ptr<AssignStmt> makeAssign(string name, unique_ptr<Expr> val) {
    return make_unique<AssignStmt>(move(name), move(val));
}

unique_ptrExprStmt> makeExprStmt(unique_ptr<Expr> expr) {
    return make_unique<Exprmt>(move(expr));
}

unique_ptr<BreakStmt> makeBreak() {
    return make_unique<BreakStmt>();
}

unique_ptr<IfStmt> makeIf(unique_ptr<Expr> cond, unique_ptr<Stmt> thenBranch,
                           unique_ptr<Stmt> elseBranch = nullptr) {
    return make_unique<IfStmt>(move(cond), move(thenBranch), move(elseBranch));
}

template <typename... Stmts>
unique_ptr<Block> makeBlock(Stmts&&... stmts) {
    auto block = make_unique<Block>();
    if constexpr (sizeof...(Stmts) > 0) {
       block->statements.push_back(forward<Stmts>(stmts)), ...);
    }
    return block;
}

unique_ptr<Decl> makeVarDecl name, unique_ptr<Expr> init                                 bool isGlobal = false) {
    return make_unique<VarDecl>(move(name), move(init), isGlobal);
}

unique_ptr<ConstDecl> makeConst(string name, unique_ptrExpr> init,
                                     bool isGlobal = false) {
    return make_unique<ConstDecl>(move(name),(init), isGlobal);
}

unique_ptr<Param> makeParam(string name) {
    return make_unique<Param>(move(name));
}

unique_ptr<FuncDef> makeFuncDef(string name,* retType,
                                 vector<unique_ptr<Param>> params,
                                 unique_ptr<Block> body) {
    return make_unique<FuncDef>(retType, move(name), move(params), move(body));
}

template <typename... Elems>
unique_ptr<CompUnit> makeCompUnit(Elems&&... elems) {
    auto unit = make_unique<CompUnit>();
    if constexpr (sizeof...(Elems) > 0) {
        (unit->elements.push_back(forward<Elems>(elems)), ...);
    }
    return unit;
}

// =====================================================================
// Test utilities
// =====================================================================

bool hasError(const vector<SemanticError>& errors, SemanticErrorCode code) {
    (const auto& e : errors) {
        if (e.code == code) return true;
    }
    return false;
}

void printErrors(const vector<SemanticError>& errors) {
    for (const auto& e : errors) {
        cout << "    Error " << e.message << " (code="
             << static_cast<int>(e.code) << ")" << endl;
    }
}

int_failCount = 0;

void runTest(const string& name, unique_ptr<CompUnit> unit,
             bool expectValid,
             SemanticErrorCode expectedError = SemanticErrorCode::ERR_UNDECLAREDENT) {
    SemanticAnalyzer analyzer;
    bool result = analyzer.analyze(unit.get());
    const auto& errors = analyzer.errors();

    bool pass = false;
    if (expectValid) {
        pass = result && !analyzer.hasErrors();
    } {
        pass = !result && hasError(errors, expectedError);
    }

    if (pass) {
        cout <<PASS] " << name << endl;
    } else {
        cout << "[FAIL] " << name << endl;
        if (expectValid) {
            cout << "  Expected: no errors (analyze() returned "
                 << ( ? "true" : "false") << ")" << endl;
        } else {
            cout << "  Expected error code:                 << static_cast<int>(expectedError) << endl            cout << "  analyze() returned: " << (result ? "true" "false")
                 << endl;
        }
        if (!errors.empty()) {
            cout << "  Errors found:" << endl;
            printErrors(errors);
        } else {
            cout << "  (no errors reported)" << endl;
        }
        g_fCount++;
    }
}

 =====================================================================
// main
// =====================================================================

int main() {
    cout << "=== Semanticzer Test Harness ===" << endl << endl;

    // ----------------------------------------------------------------
    // Test 1: Empty main (valid)
    //   int main(void) { return 0; }
    // ----------------------------------------------------------------
    {
        auto unit = makeCompUnit(
            makeFuncDef("main", Type::intType(), {},
                        makeBlock(makeReturn(makeNumber(0)))));
        runTest("Test 1: Empty main (valid)", move(unit), true);
    }

    // ----------------------------------------------------------------
    // Test 2: Undeclared (invalid, ERR_ECLARED_IDENT)
    //   int main(void) { return x; }
    // ----------------------------------------------------------------
    {
        auto unit = makeCompUnit(
            makeFuncDef("main", Type::intType(), {},
                        makeBlock(makeReturn(makeId("x")))));
        runTest(" 2: Undeclared variable",
                move(unit), false, SemanticErrorCode::ERR_UNDECLARED_IDENT);
    }

    // ----------------------------------------------------------------
    // Test 3: Const reassign (invalid, ERRST_REASSIGN)
    //   const int A = 5; int main) { A = 10; return 0; }
    // -------------------------------------------------------------    {
        auto unit = makeCompUnit(
           ConstDecl("A", makeNumber(5), true),
            makeFuncDef("main", Type::intType(), {},
                        makeBlock(makeAssign("A", makeNumber(10)),
                                  makeReturn(makeNumber(0)))));
        runTest("Test 3: Const reassign",
                move(unit), false, SemanticErrorCode::ERR_CONST_REASSIGN);
    }

    // -------------------------------------------------------------    // Test 4: Main signature wrong (invalid, ERR_INVALID_MAIN_SIGNATURE)
    //   void main(void) {}
    // ----------------------------------------------------------------
    {
        auto unit = makeCompUnit(
            makeFuncDef("main", Type::voidType(), {}, makeBlock()));
        runTest("Test 4: Main signature wrong",
                move(unit false,
                SemanticErrorCode::ERR_INVALID_MAIN_SIGNATURE);
 }

    // ----------------------------------------------------------------
    // Test 5: Missing return (invalid, ERR_MISSING_RETURN)
    //   int f(int x) { if (x > 0) 1; }
    //   int main(void) { return 0; }
    // ----------------------------------------------------------------
    {
        auto fBody = makeBlock(
            makeIf(makeBin(makeId("x"), BinaryExpr::Op::GT, makeNumber(0)),
                   makeReturn(makeNumber(1))));
        auto f = makeFuncDef("f", Type::intType(), {makeParam("x")},
                             move(fBody));
        auto mainBody = makeBlock(makeReturn(makeNumber(0)));
        auto mainFunc = makeFuncDef("main Type::intType(), {},
                                    move(mainBody        auto unit = makeCompUnit(move(f move(mainFunc));
        runTest("Test 5: Missing return",
                move(unit), false SemanticErrorCode::ERR_MISSING_RETURN);
    }

    // ----------------------------------------------------------------
    // Test 6: Break outside loop (invalid, ERR_BREAK_OUTSIDE_LOOP)
    //   int main(void) { break; return 0; }
    // ----------------------------------------------------------------
           auto unit = makeUnit(
            makeFunc("main", Type::intType(), {},
                        makeBlock(makeBreak(), makeReturn(makeNumber(0)))));
        runTest("Test 6: Break outside loop",
                move(unit), false,
                SemanticErrorCode::ERR_BREAK_OUTSIDE_LOOP    }

    // ----------------------------------------------------------------
    // Test 7: Division by zero (, ERR_DIVIDE_BY_ZERO)
    //   int main(void) return 1/0; }
    // ----------------------------------------------------------------
    {
        auto unit = makeCompUnit(
            makeFuncDef("main", Type::intType(), {},
                        makeBlock(makeReturn(
                            makeBin(makeNumber(1), BinaryExpr::Op::DIV,
                                    makeNumber(0));
        runTest("Test 7 Division by zero",
                move(unit), false, SemanticErrorCode::ERR_DIVIDE_BY_ZERO);
    }

    // ----------------------------------------------------------------
    // Test8: Duplicate global (invalid, ERR_REDEFINED_SYMBOL)
    //   int x = 1; int x =2; int main(void) { return 0; }
    // ----------------------------------------------------------------
    {
        auto unit = makeCompUnit(
            makeVarDecl("x", makeNumber(1), true),
            makeVarDecl("x", makeNumber(2), true),
            makeFuncDef("main", Type::intType(), {},
                        makeBlock(makeReturn(makeNumber(0)))));
        runTest("Test 8: Duplicate global",
                move(unit), false SemanticErrorCode::ERR_REDEFINED_SYMBOL);
    }

    // ----------------------------------------------------------------
    // Test 9: Full valid complex program (valid)
    //   const int N = 10;
    //   int add(int a, int b) { return a + b; }
    //   int main(void) { int x 5; if (x > 0) return add(x, N); return 0; }
    // ----------------------------------------------------------------
    {
        vector<unique_ptr<Expr>> addArgs;
        addArgs.push_back(makeId("x"));
        addArgs.push_back(makeId("N"));

        auto addFunc = makeFuncDef("add", Type::intType(),
                                   {makeParam("a"), makeParam("b")},
                                   makeBlock(makeReturn(
                                       makeBin(makeId("a"), BinaryExpr::Op::,
                                               makeIdb")))));

        auto main = makeFuncDef("main", Type::intType(), {},
                                    makeBlock(
                                        makeVarDecl("x", makeNumber(5), false),
                                        makeIf(makeBin(makeId("x"),
                                                       BinaryExpr::Op::GT,
                                                       makeNumber(0)),
                                               makeReturn(makeCall("add",
                                                                   move(addArgs)))),
                                       Return(makeNumber(0))));

        auto unit = makeCompUnit(
            makeConstDecl("N", makeNumber(10),),
            move(addFunc),
            move(mainFunc));
        runTest("Test 9: Full valid complex program", move(unit), true);
    }

    // ----------------------------------------------------------------
    // Test 10: Wrong arg count (invalid, ERR_WRONG_ARG_COUNT)
    //   int f(int x) { return x; }
    //   main(void) { return f(1, 2); }
    // ----------------------------------------------------------------
    {
        vector<unique_ptr<Expr>> callArgs;
        callArgs.push_back(makeNumber(1));
        callArgs.push_back(makeNumber(2));

        auto f = makeFuncDef("f",::intType(), {makeParam("x")},
                             makeBlock(makeReturn(makeId("x));
        auto mainFunc = makeFuncDef("main", Type::intType(), {},
                                    makeBlock(makeReturn(
                                        makeCall("f", move(call)))));
        auto unit = makeCompUnit(move(f), move(mainFunc));
        runTest("Test 10: Wrong arg count",
                move(unit), false, SemanticErrorCode::ERR_WRONG_ARG_COUNT);
    }

    // ================================================================
    Summary
    // ================================================================
    cout << endl;
    if (g_failCount == ) {
        cout << "All tests passed!" << endl;
        return 0;
    } else {
        << g_failCount << " test(s) FAILED." << endl;
        return 1;
    }
}
'''

path = r"F:\Codes\Compiler_FullVersion\teststest_semantic.cppwith open(path, "w", encoding="utf-") as f:
   .write(content)
print(f"Written {len(content)} bytes to {path}")
