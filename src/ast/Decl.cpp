#include "ast/Decl.h"
#include "ast/AstVisitor.h"

void VarDecl::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void ConstDecl::accept(AstVisitor& visitor) const { visitor.visit(*this); }
