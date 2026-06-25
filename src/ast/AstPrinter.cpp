#include "ast/AstPrinter.h"

void AstPrinter::indent() {
    for (int i = 0; i < depth_; ++i) {
        out_ << "  ";
    }
}

void AstPrinter::push() { ++depth_; }
void AstPrinter::pop() { --depth_; }

void AstPrinter::print(const toyc::CompUnit& node) {
    visit(node);
}

void AstPrinter::visit(const toyc::CompUnit& node) {
    out_ << "CompUnit\n";
    push();
    for (const auto& item : node.elements) {
        indent();
        std::visit([this](const auto& ptr) { ptr->accept(*this); }, item);
    }
    pop();
}

void AstPrinter::visit(const toyc::VarDecl& node) {
    out_ << "VarDecl " << node.name << (node.isGlobal ? " global" : " local") << "\n";
    push();
    indent();
    node.initExpr->accept(*this);
    pop();
}

void AstPrinter::visit(const toyc::ConstDecl& node) {
    out_ << "ConstDecl " << node.name << (node.isGlobal ? " global" : " local") << "\n";
    push();
    indent();
    node.initExpr->accept(*this);
    pop();
}

void AstPrinter::visit(const toyc::FuncDef& node) {
    out_ << "FuncDef " << node.returnType->name() << " " << node.name << "\n";
    push();
    for (const auto& param : node.params) {
        indent();
        param->accept(*this);
    }
    indent();
    node.body->accept(*this);
    pop();
}

void AstPrinter::visit(const toyc::Param& node) {
    out_ << "Param " << node.name << "\n";
}

void AstPrinter::visit(const toyc::Block& node) {
    out_ << "Block\n";
    push();
    for (const auto& stmt : node.statements) {
        indent();
        stmt->accept(*this);
    }
    pop();
}

void AstPrinter::visit(const toyc::ExprStmt& node) {
    out_ << "ExprStmt\n";
    push();
    indent();
    node.expr->accept(*this);
    pop();
}

void AstPrinter::visit(const toyc::AssignStmt& node) {
    out_ << "Assign\n";
    push();
    indent();
    node.lvalue->accept(*this);
    indent();
    node.value->accept(*this);
    pop();
}

void AstPrinter::visit(const toyc::IfStmt& node) {
    out_ << "If\n";
    push();
    indent();
    out_ << "Cond\n";
    push();
    indent();
    node.cond->accept(*this);
    pop();
    indent();
    out_ << "Then\n";
    push();
    indent();
    node.then->accept(*this);
    pop();
    if (node.has_else()) {
        indent();
        out_ << "Else\n";
        push();
        indent();
        node.else_->accept(*this);
        pop();
    }
    pop();
}

void AstPrinter::visit(const toyc::WhileStmt& node) {
    out_ << "While\n";
    push();
    indent();
    out_ << "Cond\n";
    push();
    indent();
    node.cond->accept(*this);
    pop();
    indent();
    out_ << "Body\n";
    push();
    indent();
    node.body->accept(*this);
    pop();
    pop();
}

void AstPrinter::visit(const toyc::BreakStmt& /*node*/) {
    out_ << "Break\n";
}

void AstPrinter::visit(const toyc::ContinueStmt& /*node*/) {
    out_ << "Continue\n";
}

void AstPrinter::visit(const toyc::ReturnStmt& node) {
    out_ << "Return\n";
    if (node.has_value()) {
        push();
        indent();
        node.value->accept(*this);
        pop();
    }
}

void AstPrinter::visit(const toyc::BinaryExpr& node) {
    out_ << "Binary " << toyc::binary_op_name(node.op) << "\n";
    push();
    indent();
    node.left->accept(*this);
    indent();
    node.right->accept(*this);
    pop();
}

void AstPrinter::visit(const toyc::UnaryExpr& node) {
    out_ << "Unary " << toyc::unary_op_name(node.op) << "\n";
    push();
    indent();
    node.operand->accept(*this);
    pop();
}

void AstPrinter::visit(const toyc::Number& node) {
    out_ << "Number " << node.value << "\n";
}

void AstPrinter::visit(const toyc::Identifier& node) {
    out_ << "Ident " << node.name << "\n";
}

void AstPrinter::visit(const toyc::FuncCall& node) {
    out_ << "FuncCall " << node.name << (node.isStatement ? " stmt" : "") << "\n";
    push();
    for (const auto& arg : node.args) {
        indent();
        arg->accept(*this);
    }
    pop();
}
