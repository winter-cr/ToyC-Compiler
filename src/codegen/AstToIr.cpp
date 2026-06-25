#include "codegen/AstToIr.h"

namespace toyc {

namespace backend = toyc::backend;

AstToIr::AstToIr() = default;

backend::Program AstToIr::convert(CompUnit* ast) {
    program_ = backend::Program{};
    global_names_.clear();
    if (ast) {
        ast->accept(*this);
    }
    return std::move(program_);
}

// ---- Helpers ----

backend::BinaryOp AstToIr::mapBinaryOp(toyc::BinaryOp op) {
    switch (op) {
        case toyc::BinaryOp::Add: return backend::BinaryOp::Add;
        case toyc::BinaryOp::Sub: return backend::BinaryOp::Subtract;
        case toyc::BinaryOp::Mul: return backend::BinaryOp::Multiply;
        case toyc::BinaryOp::Div: return backend::BinaryOp::Divide;
        case toyc::BinaryOp::Mod: return backend::BinaryOp::Remainder;
        case toyc::BinaryOp::Eq:  return backend::BinaryOp::Equal;
        case toyc::BinaryOp::Ne:  return backend::BinaryOp::NotEqual;
        case toyc::BinaryOp::Lt:  return backend::BinaryOp::Less;
        case toyc::BinaryOp::Le:  return backend::BinaryOp::LessEqual;
        case toyc::BinaryOp::Gt:  return backend::BinaryOp::Greater;
        case toyc::BinaryOp::Ge:  return backend::BinaryOp::GreaterEqual;
        default: return backend::BinaryOp::Add;
    }
}

backend::UnaryOp AstToIr::mapUnaryOp(toyc::UnaryOp op) {
    switch (op) {
        case toyc::UnaryOp::Minus: return backend::UnaryOp::Negate;
        case toyc::UnaryOp::Not:   return backend::UnaryOp::LogicalNot;
        default: return backend::UnaryOp::Negate;
    }
}

std::optional<backend::Value> AstToIr::lookup(const std::string& name) const {
    auto it = locals_.find(name);
    if (it != locals_.end()) {
        return it->second;
    }
    if (global_names_.count(name)) {
        return backend::Value::global(name);
    }
    return std::nullopt;
}

// ---- Expression emission ----

backend::Value AstToIr::emitExpr(Expr* expr, backend::FunctionBuilder& builder) {
    if (!expr) {
        return backend::Value::imm(0);
    }

    if (auto* n = dynamic_cast<Number*>(expr)) {
        return backend::Value::imm(n->value);
    }

    if (auto* id = dynamic_cast<Identifier*>(expr)) {
        auto val = lookup(id->name);
        if (val.has_value()) {
            return val.value();
        }
        return backend::Value::imm(0);
    }

    if (auto* bin = dynamic_cast<BinaryExpr*>(expr)) {
        if (bin->op == toyc::BinaryOp::And) {
            backend::Value left = emitExpr(bin->left.get(), builder);
            return builder.emitLogicalAnd(left,
                [this, &builder, raw = bin->right.get()](backend::FunctionBuilder&) {
                    return emitExpr(raw, builder);
                });
        }
        if (bin->op == toyc::BinaryOp::Or) {
            backend::Value left = emitExpr(bin->left.get(), builder);
            return builder.emitLogicalOr(left,
                [this, &builder, raw = bin->right.get()](backend::FunctionBuilder&) {
                    return emitExpr(raw, builder);
                });
        }
        backend::Value left = emitExpr(bin->left.get(), builder);
        backend::Value right = emitExpr(bin->right.get(), builder);
        backend::Value temp = builder.createTemporary();
        builder.emit(backend::Instruction::binary(temp, mapBinaryOp(bin->op), left, right));
        return temp;
    }

    if (auto* un = dynamic_cast<UnaryExpr*>(expr)) {
        backend::Value operand = emitExpr(un->operand.get(), builder);
        if (un->op == toyc::UnaryOp::Plus) {
            return operand;
        }
        backend::Value temp = builder.createTemporary();
        builder.emit(backend::Instruction::unary(temp, mapUnaryOp(un->op), operand));
        return temp;
    }

    if (auto* call = dynamic_cast<FuncCall*>(expr)) {
        std::vector<backend::Value> args;
        for (auto& arg : call->args) {
            args.push_back(emitExpr(arg.get(), builder));
        }
        backend::Value temp = builder.createTemporary();
        builder.emit(backend::Instruction::call(temp, call->name, std::move(args)));
        return temp;
    }

    return backend::Value::imm(0);
}

// ---- Visitor: CompUnit (two-pass: globals first, then functions) ----

void AstToIr::visit(const CompUnit& node) {
    // Pass 1: collect all global declarations so functions can reference them
    for (auto& elem : node.elements) {
        if (auto* var = std::get_if<std::unique_ptr<VarDecl>>(&elem)) {
            if (*var && (*var)->isGlobal) (*var)->accept(*this);
        } else if (auto* cnst = std::get_if<std::unique_ptr<ConstDecl>>(&elem)) {
            if (*cnst && (*cnst)->isGlobal) (*cnst)->accept(*this);
        }
    }
    // Pass 2: compile functions (which may reference globals from pass 1)
    for (auto& elem : node.elements) {
        if (auto* var = std::get_if<std::unique_ptr<VarDecl>>(&elem)) {
            if (*var && !(*var)->isGlobal) (*var)->accept(*this);
        } else if (auto* cnst = std::get_if<std::unique_ptr<ConstDecl>>(&elem)) {
            if (*cnst && !(*cnst)->isGlobal) (*cnst)->accept(*this);
        } else if (auto* func = std::get_if<std::unique_ptr<FuncDef>>(&elem)) {
            if (*func) (*func)->accept(*this);
        }
    }
}

// ---- Constant expression evaluation for global initializers ----
namespace {

std::optional<int> evalGlobalInit(Expr* expr,
    const std::vector<backend::Global>& globals) {
    if (!expr) return std::nullopt;
    if (auto* n = dynamic_cast<Number*>(expr)) return n->value;
    if (auto* id = dynamic_cast<Identifier*>(expr)) {
        for (auto& g : globals) {
            if (g.name == id->name) return g.initial_value;
        }
        return std::nullopt;
    }
    if (auto* bin = dynamic_cast<BinaryExpr*>(expr)) {
        auto l = evalGlobalInit(bin->left.get(), globals);
        auto r = evalGlobalInit(bin->right.get(), globals);
        if (!l || !r) return std::nullopt;
        switch (bin->op) {
            case toyc::BinaryOp::Add: return *l + *r;
            case toyc::BinaryOp::Sub: return *l - *r;
            case toyc::BinaryOp::Mul: return *l * *r;
            case toyc::BinaryOp::Div: return (r && *r != 0) ? std::optional(*l / *r) : std::nullopt;
            case toyc::BinaryOp::Mod: return (r && *r != 0) ? std::optional(*l % *r) : std::nullopt;
            default: return std::nullopt;
        }
    }
    return std::nullopt;
}

} // namespace

// ---- Visitor: Declarations ----

void AstToIr::visit(const VarDecl& node) {
    if (node.isGlobal) {
        int initVal = 0;
        if (node.initExpr) {
            if (auto val = evalGlobalInit(node.initExpr.get(), program_.globals)) {
                initVal = *val;
            }
        }
        program_.globals.push_back(backend::Global{node.name, initVal, false});
        global_names_[node.name] = true;
    } else {
        if (builder_) {
            backend::Value local = builder_->createLocal(node.name);
            locals_[node.name] = local;
            if (node.initExpr) {
                backend::Value initVal = emitExpr(node.initExpr.get(), *builder_);
                builder_->emit(backend::Instruction::copy(local, initVal));
            }
        }
    }
}

void AstToIr::visit(const ConstDecl& node) {
    if (node.isGlobal) {
        int initVal = 0;
        if (node.initExpr) {
            if (auto val = evalGlobalInit(node.initExpr.get(), program_.globals)) {
                initVal = *val;
            }
        }
        program_.globals.push_back(backend::Global{node.name, initVal, true});
        global_names_[node.name] = true;
    } else {
        if (builder_) {
            backend::Value local = builder_->createLocal(node.name);
            locals_[node.name] = local;
            if (node.initExpr) {
                backend::Value initVal = emitExpr(node.initExpr.get(), *builder_);
                builder_->emit(backend::Instruction::copy(local, initVal));
            }
        }
    }
}

// ---- Visitor: FuncDef ----

void AstToIr::visit(const FuncDef& node) {
    locals_.clear();
    current_func_name_ = node.name;

    backend::FunctionBuilder funcBuilder(node.name, node.returnType->is_int());

    builder_ = &funcBuilder;

    for (auto& param : node.params) {
        param->accept(*this);
    }

    if (node.body) {
        node.body->accept(*this);
    }

    if (node.returnType->is_void()) {
        funcBuilder.emit(backend::Instruction::ret());
    }

    builder_ = nullptr;
    program_.functions.push_back(std::move(funcBuilder).finish());
}

void AstToIr::visit(const Param& node) {
    if (builder_) {
        backend::Value paramVal = builder_->addParameter(node.name);
        locals_[node.name] = paramVal;
    }
}

// ---- Visitor: Statements ----

void AstToIr::visit(const Block& node) {
    // Save/restore locals_ for block scope: variables declared inside
    // this block shadow outer ones and go out of scope on exit.
    auto saved_locals = locals_;
    for (auto& stmt : node.statements) {
        if (stmt) {
            stmt->accept(*this);
        }
    }
    locals_ = std::move(saved_locals);
}

void AstToIr::visit(const IfStmt& node) {
    if (!builder_) return;

    backend::Value cond = emitExpr(node.cond.get(), *builder_);
    builder_->emitIf(cond,
        [this, &node](backend::FunctionBuilder& fb) {
            builder_ = &fb;
            if (node.then) node.then->accept(*this);
        },
        [this, &node](backend::FunctionBuilder& fb) {
            builder_ = &fb;
            if (node.else_) node.else_->accept(*this);
        });
}

void AstToIr::visit(const WhileStmt& node) {
    if (!builder_) return;

    builder_->emitWhile(
        [this, &node](backend::FunctionBuilder& fb) -> backend::Value {
            builder_ = &fb;
            return emitExpr(node.cond.get(), fb);
        },
        [this, &node](backend::FunctionBuilder& fb) {
            builder_ = &fb;
            if (node.body) node.body->accept(*this);
        });
}

void AstToIr::visit(const BreakStmt& /*node*/) {
    if (builder_) builder_->emitBreak();
}

void AstToIr::visit(const ContinueStmt& /*node*/) {
    if (builder_) builder_->emitContinue();
}

void AstToIr::visit(const ReturnStmt& node) {
    if (!builder_) return;
    if (node.value) {
        backend::Value val = emitExpr(node.value.get(), *builder_);
        builder_->emit(backend::Instruction::ret(val));
    } else {
        builder_->emit(backend::Instruction::ret());
    }
}

void AstToIr::visit(const ExprStmt& node) {
    if (!builder_ || !node.expr) return;
    // Route void function calls to statement-level handler (no return
    // value capture), all other expressions through emitExpr.
    if (auto* call = dynamic_cast<FuncCall*>(node.expr.get())) {
        if (call->isStatement) {
            visit(*call);
            return;
        }
    }
    emitExpr(node.expr.get(), *builder_);
}

void AstToIr::visit(const AssignStmt& node) {
    if (!builder_) return;

    backend::Value rhs = emitExpr(node.value.get(), *builder_);
    auto lhs = lookup(node.lvalue->name);
    if (lhs.has_value()) {
        builder_->emit(backend::Instruction::copy(lhs.value(), rhs));
    } else {
        builder_->emit(backend::Instruction::copy(backend::Value::global(node.lvalue->name), rhs));
    }
}

void AstToIr::visit(const BinaryExpr& /*node*/) {
    // Handled by emitExpr
}

void AstToIr::visit(const UnaryExpr& /*node*/) {
    // Handled by emitExpr
}

void AstToIr::visit(const Identifier& /*node*/) {
    // Handled by emitExpr
}

void AstToIr::visit(const Number& /*node*/) {
    // Handled by emitExpr
}

void AstToIr::visit(const FuncCall& node) {
    // Statement-level void function call
    if (!builder_) return;

    std::vector<backend::Value> args;
    for (auto& arg : node.args) {
        args.push_back(emitExpr(arg.get(), *builder_));
    }
    builder_->emit(backend::Instruction::call(std::nullopt, node.name, std::move(args)));
}

} // namespace toyc
