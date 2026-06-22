#include "ast/Stmt.h"
#include "ast/AstVisitor.h"

void BlockStmt::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void EmptyStmt::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void ExprStmt::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void AssignStmt::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void DeclStmt::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void IfStmt::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void WhileStmt::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void BreakStmt::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void ContinueStmt::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void ReturnStmt::accept(AstVisitor& visitor) const { visitor.visit(*this); }
