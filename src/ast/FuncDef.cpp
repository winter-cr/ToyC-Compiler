#include "ast/FuncDef.h"
#include "ast/AstVisitor.h"

void Param::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void FuncDef::accept(AstVisitor& visitor) const { visitor.visit(*this); }
