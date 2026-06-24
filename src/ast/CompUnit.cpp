#include "ast/CompUnit.h"
#include "ast/AstVisitor.h"

void CompUnit::accept(AstVisitor& visitor) const { visitor.visit(*this); }
