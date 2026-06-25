import subprocess, sys, os

SRC = r'F:\Codes\Compiler_FullVersion'
TC = os.path.join(SRC, 'tests')
BD = os.path.join(SRC, 'build')
INC = os.path.join(SRC, 'src')

CPP = os.path.join(TC, 'test_prog.cpp')
EXE = os.path.join(BD, 'test_prog.exe')

code = r'''#include "ast/AST.h"
#include "semantic/SemanticAnalyzer.h"
#include <iostream>
#include <memory>
using namespace toyc;
using namespace std;

int g_pass=0,g_fail=0;

bool H(const auto& E, auto C) { for(auto& e:E) if(e.code==C) return 1; return 0; }

void T(const string& N, auto CU, bool OK, auto EC) {
    SemanticAnalyzer A; bool R=A.analyze(CU.get());
    bool P = OK ? (R && !A.hasErrors()) : (!R && H(A.errors(),EC));
    if(P) { cout<<"[PASS] "<<N<<endl; g_pass++; }
    else { cout<<"[FAIL] "<<N<<endl; cout<<"  r="<<R<<" e="<<A.errorssize()<<endl;
        for(auto& e:A.errors()) cout<<"    "<<e.message<<endl; g_fail++; }
}

auto F(const string& N, Type* T, auto B) {
    vector<unique_ptr<Param>> P; return make_unique<FuncDef>(T,N,move(P),move(B));
}

auto R(int V) { return make_unique<ReturnStmt>(make_uniqueNumber>(V)); }

int main() {
    cout<<"=== Tests ==="<<endl;

    // 1
    { auto B=make_unique<Block>(); B->statements.push_back(R(0));
      auto CU=make_unique<CompUnit> CU->elements.push_back(F("main",Type::intType(),move(B)));
      T("1 valid main",move(CU),1,{}); }

    // 2    { auto B=make_unique<Block>(); B->statements.push_back_unique<ReturnStmtmake_unique<Identifier>("x")));
      auto CU=make_unique<Comp>(); CU->elements.push_back(F("main",Type::intType(),move(B)));
      T("2 undeclared x",move(CU),0,SemanticErrorCode::ERR_UNDECLARED_IDENT); }

    // 3 break outside
    { auto B=make_unique<Block>(); B->statements.push_back(make_unique<BreakStmt>()); B->statements.push_back(R(0));
      auto CU=make_unique<CompUnit>(); CU->elements.push_back(F("main",::intType(),move(B)));
      T("3 break outside",move(CU),0,SemanticErrorCode::ERR_BREAK_OUTSIDE_LOOP }

    // 4 const reassign
    { auto CU=make_unique<Unit>(); CU->elements_back(make_unique<ConstDecl>("A",make_unique<Number>(5),1));
      auto B=make_unique<Block>(); B->statements.push_back(make_unique<AssignSt>("A",make_unique<Number>(10); B->statements.push_back(R(0));
      CU->elements.push_back(F("main",Type::intType(),move(B)));
      T("4 const reassign",move(CU),0emanticErrorCode::ERR_CONST_REASSIGN); }

    // 5 void main
    { auto CU=make_unique<CompUnit>(); CUelements.push_back(F("main",Type::voidType(),make_unique<Block>()));
      T(" void main",move(CU),0,SemanticErrorCode::ERR_INVALID_MAIN_SIGN); }

    // 6 continue outside
    { auto B=make_unique<>(); B->stat.push_back(make_unique<ContinueStmt>()); B->statements.push_back(0));
      auto CU=make_unique<CompUnit>(); CU->elements.push_back(F("main",Type::intType(),move(B)));
      T("6 continue outside",move(CU),0,SemanticErrorCode::ERR_CONTINUE_OUTSIDE_LOOP); }

    // 7 div by zero
    { auto B=make_unique<Block>(); B->statements.push_back(make_unique<ReturnStmt>(
        make_unique<BinaryExpr>(make_unique<Number>(1),BinaryExpr::Op::DIV,make_unique<Number>(0))));
      auto CU=make_unique<CompUnit>(); CU->elements.push_back(F("main",Type::intType(),move(B)));
      T("7 div 0",move(CU),0,SemanticErrorCode::ERR_DIVIDE_BY_ZERO); }

    // 8 dup global
    { auto CU=make_unique<CompUnit>(); CU->elements.push_back(make_unique<VarDecl>("x",make_unique<Number>(1),1));
      CU->elements.push_back(make<VarDecl>("x",make_unique<Number>(2),1));
      CU->elements.push_back("main",Type::intType(),[&]{auto b=make_unique<Block>(); b->statements.push_back(R(0)); return b;}()));
      T("8 dup global",move(CU),0,SemanticErrorCode::ERR_REDEFINEDYMBOL); }

    // 9 break in while = ok
    { auto W=make_unique<Block>(); W->statements.push_back(make_unique<BreakStmt>());
      auto B=make_unique<Block>(); B->statements.push_back(make_unique<Stmt>(make_unique<Number>(1move(W))); B->statements.push_back(R(0));
      auto CU=make_unique<CompUnit>(); CU->elements.push_back(F("main",Type::intType(),move(B)));
      T("9 break in while",move(CU),1,{}); }

    10 dup param
    { auto CU=make_unique<CompUnit>();
      vector<unique_ptr<Param>>; FP.push_back(make_unique<>("x")); FP.push_back(make_unique<Paramx"));
      auto FBmake_unique<Block>(); FB->statements.push_back(make_unique<ReturnStmt>(make_unique<Identifier>("x")));
      CU->elements.push_back(make_unique<FuncDef>(Type::intType(),"f",move(FP),move(FB)));
      CU->elements.push_back(F("",Type::intType(),[&]{auto b=make_unique<Block>();->statements.push_back(R(0)); return b;}()));
      T("10 dup param",(CU),0,SemanticErrorCode::ERR_REDEFINEDYMBOL); }

    // 11 return in
    { auto CU=make_unique<CompUnit>();
      CU->elements.push_back(F("f",Type::void(),[&]{auto b=make_unique<Block>(); b->statements.push_back(make_unique<ReturnStmt>(make_unique<Number>(1))); return b;}()));
      CU->elements.push_back(F("main",Type::intType(),[&]{auto b=make_unique<Block>(); b->statements.push_back(R(0)); return b;}()));
      T("11 ret in void",move(CU),0emanticErrorCode::ERR_RETURN_TYPE_MISMATCH); }

    // 12 global var valid
    { auto CU=make_unique<CompUnit>(); CU->elements.push_back(make_unique<VarDecl>("",make_unique<Number>(),1));
      CU->elements.push_back(F("main",Type::intType(),[&]{auto b=make_unique<Block>(); b->stat.push_back(make_unique<ReturnStmt>(make_unique<Identifier>("x"))); return b;}()));
      T("12 global var",move(CU),1,{}); }

    // 13 missing return
    { auto CU=make<CompUnit>();
      vector<unique_ptr<Param>> FP; FP.push_back(make_unique<Param>("x"));
      auto FB=make_unique<Block>(); FB->statements.push_back(make_unique<IfStmt>(
        make_unique<Binary>(make_unique<Identifier>("x"),BinaryExpr::Op::GT,make_unique<Number>(0        make_unique<ReturnStmt>(make_unique<Number>(1))));
      CU->elements.push_back(make_unique<FuncDef>(Type::intType(),"",move(FP),move(FB)));
      CU->elements.push_back(F("main",Type::intType(),[&]{auto b=make_unique<Block>(); b->statements.push_back(R(0)); return b;}()));
      T("13 missing ret path",move(CU),0,SemanticErrorCode::ERR_MISSING_RETURN); }

    cout<<endl<<"=== "<<g_pass<<"/"<<(g_pass+g_f)<<" passed ==="<<endl;
    return g_fail>0;
}
'''

with open(CPP,'w') as f: f.write(code)
print(f'Written: {CPP}')

r = subprocess.run(f'g++ -=c++20 -I"{INC}" "{CPP}" -c -o"{BD}/test_prog.o"', shell=Trueif r.returncode: print('COMPILE FAIL'); sys.exit()

r = subprocess.run(f'g++ -=c++20 -o"{EXE}" "{BD}/test_prog.o" '
    f'"{BDSymbol.o" "{}/SymbolTable.o"BD}/ConstExprEvaluator.o" "{BD}/Semanticzer.o"', shell)
if r.returncode: print('LINK FAIL'); sys.exit(1)

print(f'Running {EXE}...')
r = subprocess.run([EXE], capture_output=True, text=True)
print(r.stdout)
if r.stderr: print('STDERR:', r.stderr)
print(f'Exit: {r.returncode}')
