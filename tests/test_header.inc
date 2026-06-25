#include "ast/AST.h"
#include "semantic/SemanticAnalyzer.h"
#include <iostream>
#include <memory>
using namespace toyc;
using namespace std;
int g_pass = 0;
int g_fail = 0;
bool hasCode(vector<SemanticError> e, SemanticErrorCode c) {
    for (auto& x : e) if (x.code == c) return true;
    return false;
}
void t(const string& name, unique_ptr<CompUnit> cu, bool ok, SemanticErrorCode ec = (SemanticErrorCode)0) {
    SemanticAnalyzer a;
    bool r = a.analyze(cu.get());
    bool pass = ok ? (r && !a.hasErrors()) : (!r && hasCode(a.errors(), ec));
    if (pass) { cout << "  PASS: " << name << endl; g_pass++; }
    else {
        cout << "  FAIL: " << name << endl;
        cout << "    r=" << r << " errs=" << a.errors().size() << endl;
        for (auto& e : a.errors()) cout << "    " << e.message << endl;
        g_fail++;
    }
}
