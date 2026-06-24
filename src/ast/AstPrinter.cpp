#include "ast/AstPrinter.h"

void AstPrinter::indent() {
    for (int i = 0; i < depth_; ++i) {
        out_ << "  ";
    }
}

void AstPrinter::push() { ++depth_; }
void AstPrinter::pop() { --depth_; }

void AstPrinter::print(const CompUnit& node) {
    visit(node);
}

void AstPrinter::visit(const CompUnit& node) {
    out_ << "CompUnit\n";
    push();
    for (const auto& item : node.items()) {
        indent();
        std::visit([this](const auto& ptr) { ptr->accept(*this); }, item);
    }
    pop();
}

void AstPrinter::visit(const VarDecl& node) {
    out_ << "VarDecl " << node.name() << (node.is_global() ? " global" : " local") << "\n";
    push();
    indent();
    node.init().accept(*this);
    pop();
}

void AstPrinter::visit(const ConstDecl& node) {
    out_ << "ConstDecl " << node.name() << (node.is_global() ? " global" : " local") << "\n";
    push();
    indent();
    node.init().accept(*this);
    pop();
}

void AstPrinter::visit(const FuncDef& node) {
    out_ << "FuncDef " << node.return_type()->name() << " " << node.name() << "\n";
    push();
    for (const auto& param : node.params()) {
        indent();
        param->accept(*this);
    }
    indent();
    node.body().accept(*this);
    pop();
}

void AstPrinter::visit(const Param& node) {
    out_ << "Param " << node.name() << "\n";
}

void AstPrinter::visit(const BlockStmt& node) {
    out_ << "Block\n";
    push();
    for (const auto& stmt : node.stmts()) {
        indent();
        stmt->accept(*this);
    }
    pop();
}

void AstPrinter::visit(const EmptyStmt& /*node*/) {
    out_ << "EmptyStmt\n";
}

void AstPrinter::visit(const ExprStmt& node) {
    out_ << "ExprStmt\n";
    push();
    indent();
    node.expr().accept(*this);
    pop();
}

void AstPrinter::visit(const AssignStmt& node) {
    out_ << "Assign\n";
    push();
    indent();
    node.lvalue().accept(*this);
    indent();
    node.value().accept(*this);
    pop();
}

void AstPrinter::visit(const DeclStmt& node) {
    out_ << "DeclStmt\n";
    push();
    indent();
    node.decl().accept(*this);
    pop();
}

void AstPrinter::visit(const IfStmt& node) {
    out_ << "If\n";
    push();
    indent();
    out_ << "Cond\n";
    push();
    indent();
    node.condition().accept(*this);
    pop();
    indent();
    out_ << "Then\n";
    push();
    indent();
    node.then_branch().accept(*this);
    pop();
    if (node.has_else()) {
        indent();
        out_ << "Else\n";
        push();
        indent();
        node.else_branch()->accept(*this);
        pop();
    }
    pop();
}

void AstPrinter::visit(const WhileStmt& node) {
    out_ << "While\n";
    push();
    indent();
    out_ << "Cond\n";
    push();
    indent();
    node.condition().accept(*this);
    pop();
    indent();
    out_ << "Body\n";
    push();
    indent();
    node.body().accept(*this);
    pop();
    pop();
}

void AstPrinter::visit(const BreakStmt& /*node*/) {
    out_ << "Break\n";
}

void AstPrinter::visit(const ContinueStmt& /*node*/) {
    out_ << "Continue\n";
}

void AstPrinter::visit(const ReturnStmt& node) {
    out_ << "Return\n";
    if (node.has_value()) {
        push();
        indent();
        node.value()->accept(*this);
        pop();
    }
}

void AstPrinter::visit(const BinaryExpr& node) {
    out_ << "Binary " << binary_op_name(node.op()) << "\n";
    push();
    indent();
    node.lhs().accept(*this);
    indent();
    node.rhs().accept(*this);
    pop();
}

void AstPrinter::visit(const UnaryExpr& node) {
    out_ << "Unary " << unary_op_name(node.op()) << "\n";
    push();
    indent();
    node.operand().accept(*this);
    pop();
}

void AstPrinter::visit(const IntLiteral& node) {
    out_ << "Int " << node.value() << "\n";
}

void AstPrinter::visit(const IdentifierExpr& node) {
    out_ << "Ident " << node.name() << "\n";
}

void AstPrinter::visit(const CallExpr& node) {
    out_ << "Call " << node.name() << (node.is_statement() ? " stmt" : "") << "\n";
    push();
    for (const auto& arg : node.args()) {
        indent();
        arg->accept(*this);
    }
    pop();
}
