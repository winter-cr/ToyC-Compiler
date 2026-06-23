
#include "src/semantic/SemanticAnalyzer.h"
#include "src/semantic/errors.h"
#include <iostream>

using namespace semantic;
using namespace semantic::ast;

static int totalPass = 0;
static int totalFail = 0;

void testOK(const char* name, bool ok) {
    if (ok) { totalPass++; std::cout << "  [OK] " << name << std::endl; }
    else     { totalFail++; std::cout << "  [FAIL] " << name << std::endl; }
}

void testErr(const char* name, const std::vector<SemanticError>& errs,
             SemanticErrorCode want) {
    if (errs.empty()) {
        totalFail++;
        std::cout << "  [FAIL] " << name << " - no errors" << std::endl;
        return;
    }
    for (auto& e : errs) {
        if (e.code == want) {
            totalPass++;
            std::cout << "  [OK] " << name << std::endl;
            return;
        }
    }
 totalFail++;
    std::cout << "  [FAIL] " << name << " - wrong error" << std::endl;

// ===== helper factories =====
using namespace semantic;
using namespace semantic::ast;

Number* N(int v) { return new Number(1, 1, v); }
Identifier* ID(const std::string& n) { return new(1, 1, n); }
BinaryOp* BOP(const std::string& op, Expr* l, Expr* r) { return new BinaryOp(1, 1, op, l, r); }
VarDecl* VD(const std::string& n, Expr* init, bool c = false) { return new VarDecl(1, 1, n, Type::intType(), init, c); }
ConstDecl* CD(const std::string& n, Expr* init) { return new ConstDecl(1, 1, n, Type::intType(), init); }
ReturnStmt* RET(Expr* e = nullptr) { return new ReturnStmt(1, e); }
Assignment* ASGN(Expr* lv, Expr* rv) { return new Assignment(1, lv, rv); }
ExprStmt* ES(Expr* e) { return new ExprStmt(1, e); }
Block* BLK(std::vector<Stmt*> stmts) { auto* b = new Block(); b->stmts = stmts; return b; }
FuncDef* FD(const std::string& n, Type* ret, std::vector<VarDecl*> p, Block* b) { return new FuncDef(1, n, ret, p, b); }
CompUnit* CU(std::vector<Decl*> g, std::vector<FuncDef*> f) {
    auto* cu = new CompUnit(); cu->globalDecls = g; cu->functions = f; return cu;
}
BreakStmt* BRK() { return new BreakStmt(1); }
ContinueStmt* CONT() { return new ContinueStmt(1); }
WhileStmt* WHILE(Expr* c, Block* b) { return new WhileStmt(1, c, b); }
IfStmt* IF(Expr* c, Block* tb, Block* eb = nullptr) { return new IfStmt(1, c, tb, eb); }
FuncCall* CALL(const std::string& n, std::vector<Expr*> args, bool s = false) { return new FuncCall(1, 1, n, args, s); }

// ===== tests =====


void t01() {
    SemanticAnalyzer a;
    auto* cu = CU({}, { FD("main", Type::intType(), {}, BLK({ RET(N(0)) })) });
    a.analyze(cu);
    testOK("t01 empty main", !a.hasErrors());
}

void t02() {
    SemanticAnalyzer a;
    auto* cu = CU({ CD("A", N(10)) }, { FD("main", Type::intType(), {}, BLK({ RET(ID("A")) })) });
    a.analyze(cu);
    testOK("t02 global const", !a.hasErrors());
}

void t03 {
    SemanticAnalyzer a;
    auto* cu = CU({("x", N(5)),("y", BOP("+", ID("x"), N(3))) }, { FD("main", Type::intType(), {}, BLK({ RET(ID("x")) })) });
    a.analyze(cu);
    testOK("t03 global var", !a.hasErrors());
}

void t04 {
    SemanticAnalyzer a;
    auto* cu = CU({ CD("A", N(10)), CDB", BOP("+",("A"),5))), CD("C", BOP("*", ID("B"), N(2))) }, { FD("main", Type::intType(), {}, BLK({ RET(ID("C")) })) });
    a.analyze(cu);
    testOK("t04 const chain", !a.hasErrors());
}

void t05() {
    SemanticAnalyzer a;
    auto* cu = CU({ CDA", BOP("+", N(1), BOP("*", N(2), N(3)))) }, { FD("main", Type::intType(), {}, BLK({ RET(ID("A")) })) });
    a.analyze(cu);
    testOK("t05 const arithmetic", !a.hasErrors());
}

void t06() {
    SemanticAnalyzer a;
    auto* cu = CU({ CD("X", N(5)), CD("Y", N(10)), CD("C1", BOP("<", ID("X"), ID("Y"))), CD("C2", BOP("==", ID("X"), ID("Y"))) }, { FD("main", Type::intType(), {}, BL({ RET(N(0)) })) });
    a.analyze(cu);
    testOK("t06 const relations", !a.hasErrors());
}

void t07() {
    SemanticAnalyzer a;
    auto* cu = CU({ CD("Z", N(0)), CDO", N(1)), CD("A", BOP("&&", ID("Z"), N(999))), CD("B", BOP||", ID("O"), N(999))) }, { FD("main", Type::intType(), {}, BLK({ RET(BOP("+", ID("A"), ID("B"))) })) });
    a.analyze(cu);
    testOK("t07 short circuit", !a.hasErrors());
}

void t08() {
    SemanticAnalyzer a;
    auto* mb = BLK({ VD("x", N(10)), VD("y", BOP("*", ID("x"), N(2))), RET(ID("y")) });
    auto cu = CU({}, { FD("main", TypeintType(), {}, mb) });
    a.analyze(cu);
    testOK("t08 local var", !a.hasErrors());
}

void t09() {
    SemanticAnalyzer a;
    auto* mb = BLK({ VD("g", N(10 RET(ID("g")) });
    auto* cu = CU({ VD("g", N(100)) }, { FD("main", Type::intType(), {}, mb) });
    a.analyze(cu);
    test("t09 shadowing", !a.hasErrors());
}

void t10() {
    SemanticAnalyzer a;
    auto* cu = CU({}, {
        FD("foo", Type::intType(), {}, BLK({ RET(N(1)) })),
        FD("bar", Type::intType(), {}, BLK({ RET(N(2)) })),
        FD("main", Type::intType(), {}, BLK({ RET(N(0)) }))
    });
    a.analyze(cu);
    testOK("t10 multiple funcs", !a.hasErrors());
}

void t11() {
    SemanticAnalyzer a;
    auto* addP = std::vector<VarDecl*>{ VD("x", nullptr), VD("y", nullptr) };
    auto* addF = FD("add",::intType(), addP, BLK({ RET(BOP("+", ID(""), ID("y"))) }));
    auto* mb = BLK({ VD("r", CALL("add", {N(3), N(4)})), RET(ID("r")) });
    auto* cu = CU({}, { addF, FD("main", Type::intType(), {}, mb) });
    a.analyze(cu);
    testOK("t11 function call", !a.hasErrors());
}

void t12() {
 SemanticAnalyzer a;
    auto* factP = std::vector<VarDecl*>{ VD("n", nullptr) };
    auto* factB = BLK({
        IF(BOP("<=", ID("n"), N(1)), BLK({ RET(N(1)) })),
        RET(BOP("*", ID("n"), CALL("fact", {BOP("-", ID("n"), N(1))})))
    });
    auto* factF = FD("fact", Type::intType(), factP, factB);
    auto* cu = CU({}, { factF, FD("main", Type::intType(), {}, BLK({ RET(CALL("fact", {N(5)})) })) });
    a.analyze(cu    testOK("t12 recursion !a.hasErrors());
}

void t20() {
    SemanticAnalyzer a;
    auto* cu = CU({}, { FD("main", Type::intType(), {}, BLK({ RET(ID("x")) })) });
    a.analyze(cu);
    testErr("t20 undeclared var", a.errors(), SemanticErrorCode::ERR_UNDECLARED_IDENT);
}

void t21() {
    SemanticAnalyzer a;
    auto* cu =({ VD("g", N(10)), VD("g", N(20)) }, { FD("main", Type::intType(), {}, BLK({ RET(N(0)) })) });
    a.analyze(cu);
    testErr("t21 duplicate global", a.errors(), SemanticErrorCode::ERR_REDEFINED_SYMBOL);
}

void t22() {
    SemanticAnalyzer a;
    auto* cu = CU({ CD("A", N(5)) }, { FD("main", Type::intType(), {}, BLK({ ESG(ID("A"), N(10))), RET(ID("A")) })) });
    a.analyze(cu);
    testErr("t22 const reassign", a.errors(), SemanticErrorCode::ERR_CONST_REASSIGN);
}

void t23() {
    SemanticAnalyzer a;
    auto* cu = CU({}, { FD("main", Type::intType(), {}, BLK({ VD("x", N(1)), VD("x", N(2)), RET(ID("x")) } });
    a.analyze(cu);
    testErr("t23 duplicate local", a.errors(), SemanticCode::ERR_REDEFINED_SYMBOL);
}

void t24() {
    SemanticAnalyzer a;
    auto* cu = CU({ CD("A", BOP("+", ID("B"), N(1))) }, { FD("main", Type::intType(), {}, BLK({ RET(N(0)) })) });
    a.analyze(cu);
    testErr("t24 const undefined ref a.errors(), SemanticErrorCode::ERR_CONST_INIT_UNDEFINED}

void t25() {
    SemanticAnalyzer a;
    auto* cu = CU({ CD("A", BOP("/", N(), N(0))) }, { FD("main", Type::intType(), {}, BLK({ RET(N(0)) })) });
    a.analyze(cu);
    testErr("t25 divide by zero", a.errors(), SemanticErrorCode::ERR_DIVIDE_BY_ZERO);
}

void t26() {
    SemanticAnalyzer a;
    auto* cu = CU({ VD("x", N(5)), CD("A", ID("x")) }, { FD("main",::intType(), {}, BLK({ RET(N(0)) })) });
    a.analyze(cu);
    testErr("t26 const nonconst", a.errors(), SemanticErrorCode::ERR_CONST_EXPR_NON_CONST);
}

void t27() {
    SemanticAnalyzer a;
    auto* cu = CU({}, {});
    a.analyze(cu);
    testErr("t27 main missing", a.errors(), SemanticErrorCode::ERR_INVALID_MAIN_SIGNATURE);
}

void t28() {
    SemanticAnalyzer a;
    auto*f = FD("main", Type::voidType(), {}, BLK({ RET(); }));
    auto* cu = CU({}, { mf });
    a.analyze(cu);
    testErr("t28 main void return", a.errors(), SemanticErrorCode::ERR_INVALID_MAIN_SIGNATURE);
}

void t29() {
    SemanticAnalyzer a;
    auto* mf = FD("main", Type::intType(), { VD("argc", nullptr) }, BLK({ RET(N(0)) }));
    auto* cu = CU({}, { mf });
    a.analyze(c);
    testErr("t29 main with params", a.errors(), SemanticErrorCode::ERR_INVALID_MAIN_SIGNATURE);
}

void t30() {
    SemanticAnalyzer a;
    auto* cu = CU({}, {
        FD("foo", Type::intType(), {}, BLK({ RET(N(1)) })),
        FD("foo", Type::intType(), {}, BLK({ RET(N(2)) })),
        FD("main", Type::intType(), {}, BLK({ RET(N(0)) }))
    });
    a.analyze(cu);
    testErr("t30 function redefine", a.errors(), SemanticErrorCode::ERR_REDEFINED_SYMBOL);
}

void t31() {
    SemanticAnalyzer a;
    auto* vf = FD("foo", Type::voidType(), {}, BLK({ RET(); }));
    auto* mb = BLK({ VD("x", CALL("foo", {}, false)), RET(N(0)) });
    auto* cu = CU({}, { vf, FD("main", Type::intType(), {}, mb) });
    a.analyze(cu);
    testErr("t31 void call as expr", a.errors(), SemanticErrorCode::ERR_VOID_FUNC_CALL_EXPR);
}

 t32() {
    SemanticAnalyzer a;
    auto* cu = CU({}, { FD("main", Type::intType(), {}, BLK({ BRK(), RET(N(0)) })) });
    a.analyze(cu);
    testErr("t32 break outside loop", a.errors(), SemanticErrorCode::ERR_BREAK_OUTSIDE_LOOP);
}

void t33() {
    SemanticAnalyzer a;
    auto* cu = CU({}, { FD("main", Type::intType(), {}, BLK({ CONT(), RET(N(0)) })) });
    a.analyze(cu);
    testErr("t33 continue outside loop", a.errors(), SemanticErrorCode::ERR_CONTINUE_OUTSIDE_LOOP);
}

void t34() {
    SemanticAnalyzer a;
    auto* vf = FD("foo", TypevoidType(), {}, BLK({ RET(N(1)) }));
    auto* cu = CU({}, { vf, FD("main", Type::intType(), {}, BLK({ RET(N(0)) })) });
    a.analyze(cu);
    testErr("t34 return in void func", a.errors(), SemanticErrorCode::ERR_RETURN_TYPE_MISMATCH);
}

int main() {
    std::cout << "=== Semantic Analyzer Tests == << std::endl;
    std::cout << std::endl << "-- Valid Tests --" << std::endl;
    t01(); t02(); t03(); t04(); t05(); t06(); t07();
    t08(); t09(); t10(); t11(); t12();
    std::cout << std::endl << "-- Invalid Tests --" << std::endl;
    t20(); t21(); t22(); t23(); t24(); t25(); t26();
    t27(); t28(); t29(); t30(); t31(); t32(); t33(); t34();
    std::cout << std::endl;
    std::cout << "=== " << totalPass << " passed, " << totalFail << " failed ===" << std::endl;
    return totalFail > 0 ? 1 : 0;
}
