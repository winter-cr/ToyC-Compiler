#include "parser/Parser.h"

#include "ast/Decl.h"
#include "ast/Expr.h"
#include "ast/FuncDef.h"
#include "ast/Stmt.h"
#include "ast/Type.h"

#include <utility>

namespace {

void set_int_type(Expr& expr) {
    expr.set_type(Type::intType());
}

} // namespace

Parser::Parser(const std::vector<Token>& tokens) : tokens_(tokens) {}

std::unique_ptr<CompUnit> Parser::parse() {
    return parse_comp_unit();
}

bool Parser::is_at_end() const {
    return peek().type == TokenType::END_OF_FILE;
}

const Token& Parser::peek() const {
    return tokens_[current_];
}

const Token& Parser::previous() const {
    return tokens_[current_ - 1];
}

const Token& Parser::advance() {
    if (!is_at_end()) {
        ++current_;
    }
    return previous();
}

bool Parser::check(TokenType type) const {
    if (is_at_end()) {
        return false;
    }
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (!check(type)) {
        return false;
    }
    advance();
    return true;
}

const Token& Parser::consume(TokenType type, const char* message) {
    if (check(type)) {
        return advance();
    }
    report_error(message, current_loc());
    if (type == TokenType::SEMICOLON) {
        recover_semicolon();
    } else if (type == TokenType::RBRACE || type == TokenType::RPAREN) {
        recover_to(type);
    } else if (!is_at_end()) {
        advance();
    }
    if (check(type)) {
        return advance();
    }
    static Token placeholder(type, "", 0, 0);
    return placeholder;
}

void Parser::report_error(const char* message, SourceLocation loc) {
    errors_.emplace_back(message, loc);
}

void Parser::recover_to(TokenType type) {
    while (!is_at_end()) {
        if (peek().type == type) {
            return;
        }
        advance();
    }
}

void Parser::recover_semicolon() {
    while (!is_at_end()) {
        switch (peek().type) {
        case TokenType::SEMICOLON:
        case TokenType::RBRACE:
            return;
        case TokenType::KW_CONST:
        case TokenType::KW_INT:
        case TokenType::KW_IF:
        case TokenType::KW_WHILE:
        case TokenType::KW_BREAK:
        case TokenType::KW_CONTINUE:
        case TokenType::KW_RETURN:
        case TokenType::LBRACE:
        case TokenType::IDENT:
            return;
        default:
            advance();
        }
    }
}

void Parser::synchronize_top_level() {
    while (!is_at_end()) {
        switch (peek().type) {
        case TokenType::KW_CONST:
        case TokenType::KW_INT:
        case TokenType::KW_VOID:
            return;
        default:
            advance();
        }
    }
}

SourceLocation Parser::current_loc() const {
    return SourceLocation{peek().line, peek().column};
}

SourceLocation Parser::previous_loc() const {
    const Token& token = previous();
    return SourceLocation{token.line, token.column};
}

std::unique_ptr<CompUnit> Parser::parse_comp_unit() {
    SourceLocation loc = current_loc();
    std::vector<TopLevelItem> items;
    while (!is_at_end()) {
        items.push_back(parse_top_level_item());
    }
    return std::make_unique<CompUnit>(loc, std::move(items));
}

TopLevelItem Parser::parse_top_level_item() {
    if (check(TokenType::KW_CONST)) {
        return parse_const_decl(true);
    }
    if (check(TokenType::KW_VOID)) {
        return parse_func_def(Type::voidType());
    }
    if (check(TokenType::KW_INT)) {
        advance();
        consume(TokenType::IDENT, "expected identifier after 'int'");
        if (check(TokenType::LPAREN)) {
            current_ -= 2;
            return parse_func_def(Type::intType());
        }
        if (!check(TokenType::ASSIGN)) {
            report_error("expected '=' or '(' after identifier in declaration", current_loc());
            synchronize_top_level();
            return make_error_var_decl();
        }
        current_ -= 2;
        return parse_var_decl(true);
    }
    report_error("expected top-level declaration or function definition", current_loc());
    synchronize_top_level();
    return make_error_var_decl();
}

TopLevelItem Parser::make_error_var_decl() {
    SourceLocation loc = current_loc();
    auto init = std::make_unique<IntLiteral>(loc, 0);
    return std::make_unique<VarDecl>(loc, "__error__", std::move(init), true);
}

std::unique_ptr<VarDecl> Parser::parse_var_decl(bool is_global) {
    SourceLocation loc = current_loc();
    consume(TokenType::KW_INT, "expected 'int'");
    const Token& name = consume(TokenType::IDENT, "expected identifier");
    consume(TokenType::ASSIGN, "expected '=' in variable declaration");
    std::unique_ptr<Expr> init = parse_expr();
    consume(TokenType::SEMICOLON, "expected ';' after variable declaration");
    return std::make_unique<VarDecl>(
        SourceLocation{loc.line, loc.column}, name.lexeme, std::move(init), is_global);
}

std::unique_ptr<ConstDecl> Parser::parse_const_decl(bool is_global) {
    SourceLocation loc = current_loc();
    consume(TokenType::KW_CONST, "expected 'const'");
    consume(TokenType::KW_INT, "expected 'int' after 'const'");
    const Token& name = consume(TokenType::IDENT, "expected identifier");
    consume(TokenType::ASSIGN, "expected '=' in constant declaration");
    std::unique_ptr<Expr> init = parse_expr();
    consume(TokenType::SEMICOLON, "expected ';' after constant declaration");
    return std::make_unique<ConstDecl>(
        SourceLocation{loc.line, loc.column}, name.lexeme, std::move(init), is_global);
}

std::unique_ptr<FuncDef> Parser::parse_func_def(Type* return_type) {
    SourceLocation loc = current_loc();
    if (return_type->is_int()) {
        consume(TokenType::KW_INT, "expected 'int'");
    } else {
        consume(TokenType::KW_VOID, "expected 'void'");
    }
    const Token& name = consume(TokenType::IDENT, "expected function name");
    consume(TokenType::LPAREN, "expected '(' after function name");

    std::vector<std::unique_ptr<Param>> params;
    if (!check(TokenType::RPAREN)) {
        do {
            params.push_back(parse_param());
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RPAREN, "expected ')' after parameters");
    std::unique_ptr<BlockStmt> body = parse_block();
    return std::make_unique<FuncDef>(
        SourceLocation{loc.line, loc.column},
        return_type,
        name.lexeme,
        std::move(params),
        std::move(body));
}

std::unique_ptr<Param> Parser::parse_param() {
    SourceLocation loc = current_loc();
    consume(TokenType::KW_INT, "expected 'int' in parameter declaration");
    const Token& name = consume(TokenType::IDENT, "expected parameter name");
    return std::make_unique<Param>(SourceLocation{loc.line, loc.column}, name.lexeme);
}

std::unique_ptr<BlockStmt> Parser::parse_block() {
    SourceLocation loc = current_loc();
    consume(TokenType::LBRACE, "expected '{'");
    std::vector<std::unique_ptr<Stmt>> stmts;
    while (!check(TokenType::RBRACE) && !is_at_end()) {
        stmts.push_back(parse_stmt());
    }
    consume(TokenType::RBRACE, "expected '}'");
    return std::make_unique<BlockStmt>(SourceLocation{loc.line, loc.column}, std::move(stmts));
}

std::unique_ptr<Stmt> Parser::parse_stmt() {
    if (check(TokenType::LBRACE)) {
        return parse_block();
    }
    if (check(TokenType::SEMICOLON)) {
        SourceLocation loc = current_loc();
        advance();
        return std::make_unique<EmptyStmt>(loc);
    }
    if (check(TokenType::KW_CONST)) {
        SourceLocation loc = current_loc();
        auto decl = parse_const_decl(false);
        return std::make_unique<DeclStmt>(loc, std::move(decl));
    }
    if (check(TokenType::KW_INT)) {
        SourceLocation loc = current_loc();
        auto decl = parse_var_decl(false);
        return std::make_unique<DeclStmt>(loc, std::move(decl));
    }
    if (check(TokenType::KW_IF)) {
        return parse_if_stmt();
    }
    if (check(TokenType::KW_WHILE)) {
        return parse_while_stmt();
    }
    if (check(TokenType::KW_BREAK)) {
        SourceLocation loc = current_loc();
        advance();
        consume(TokenType::SEMICOLON, "expected ';' after 'break'");
        return std::make_unique<BreakStmt>(loc);
    }
    if (check(TokenType::KW_CONTINUE)) {
        SourceLocation loc = current_loc();
        advance();
        consume(TokenType::SEMICOLON, "expected ';' after 'continue'");
        return std::make_unique<ContinueStmt>(loc);
    }
    if (check(TokenType::KW_RETURN)) {
        return parse_return_stmt();
    }
    if (check(TokenType::IDENT)) {
        if (current_ + 1 < tokens_.size() && tokens_[current_ + 1].type == TokenType::ASSIGN) {
            SourceLocation loc = current_loc();
            const Token& name = advance();
            advance();
            std::unique_ptr<Expr> value = parse_expr();
            consume(TokenType::SEMICOLON, "expected ';' after assignment");
            auto lvalue = std::make_unique<IdentifierExpr>(
                SourceLocation{name.line, name.column}, name.lexeme);
            return std::make_unique<AssignStmt>(
                SourceLocation{loc.line, loc.column}, std::move(lvalue), std::move(value));
        }
    }

    SourceLocation loc = current_loc();
    std::unique_ptr<Expr> expr = parse_expr();
    if (auto* call = dynamic_cast<CallExpr*>(expr.get())) {
        call->set_is_statement(true);
    }
    consume(TokenType::SEMICOLON, "expected ';' after expression");
    return std::make_unique<ExprStmt>(SourceLocation{loc.line, loc.column}, std::move(expr));
}

std::unique_ptr<Stmt> Parser::parse_if_stmt() {
    SourceLocation loc = current_loc();
    consume(TokenType::KW_IF, "expected 'if'");
    consume(TokenType::LPAREN, "expected '(' after 'if'");
    std::unique_ptr<Expr> condition = parse_expr();
    consume(TokenType::RPAREN, "expected ')' after if condition");
    std::unique_ptr<Stmt> then_branch = parse_stmt();
    std::unique_ptr<Stmt> else_branch;
    if (match(TokenType::KW_ELSE)) {
        else_branch = parse_stmt();
    }
    return std::make_unique<IfStmt>(
        SourceLocation{loc.line, loc.column},
        std::move(condition),
        std::move(then_branch),
        std::move(else_branch));
}

std::unique_ptr<Stmt> Parser::parse_while_stmt() {
    SourceLocation loc = current_loc();
    consume(TokenType::KW_WHILE, "expected 'while'");
    consume(TokenType::LPAREN, "expected '(' after 'while'");
    std::unique_ptr<Expr> condition = parse_expr();
    consume(TokenType::RPAREN, "expected ')' after while condition");
    std::unique_ptr<Stmt> body = parse_stmt();
    return std::make_unique<WhileStmt>(
        SourceLocation{loc.line, loc.column}, std::move(condition), std::move(body));
}

std::unique_ptr<Stmt> Parser::parse_return_stmt() {
    SourceLocation loc = current_loc();
    consume(TokenType::KW_RETURN, "expected 'return'");
    std::unique_ptr<Expr> value;
    if (!check(TokenType::SEMICOLON)) {
        value = parse_expr();
    }
    consume(TokenType::SEMICOLON, "expected ';' after 'return'");
    return std::make_unique<ReturnStmt>(SourceLocation{loc.line, loc.column}, std::move(value));
}

std::unique_ptr<Expr> Parser::parse_expr() {
    return parse_lor_expr();
}

std::unique_ptr<Expr> Parser::parse_lor_expr() {
    SourceLocation loc = current_loc();
    std::unique_ptr<Expr> expr = parse_land_expr();
    while (match(TokenType::OR_OR)) {
        std::unique_ptr<Expr> rhs = parse_land_expr();
        auto node = std::make_unique<BinaryExpr>(
            SourceLocation{loc.line, loc.column}, BinaryOp::Or, std::move(expr), std::move(rhs));
        set_int_type(*node);
        expr = std::move(node);
        loc = current_loc();
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parse_land_expr() {
    SourceLocation loc = current_loc();
    std::unique_ptr<Expr> expr = parse_rel_expr();
    while (match(TokenType::AND_AND)) {
        std::unique_ptr<Expr> rhs = parse_rel_expr();
        auto node = std::make_unique<BinaryExpr>(
            SourceLocation{loc.line, loc.column}, BinaryOp::And, std::move(expr), std::move(rhs));
        set_int_type(*node);
        expr = std::move(node);
        loc = current_loc();
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parse_rel_expr() {
    SourceLocation loc = current_loc();
    std::unique_ptr<Expr> expr = parse_add_expr();
    while (true) {
        BinaryOp op;
        if (match(TokenType::LT)) {
            op = BinaryOp::Lt;
        } else if (match(TokenType::GT)) {
            op = BinaryOp::Gt;
        } else if (match(TokenType::LE)) {
            op = BinaryOp::Le;
        } else if (match(TokenType::GE)) {
            op = BinaryOp::Ge;
        } else if (match(TokenType::EQ)) {
            op = BinaryOp::Eq;
        } else if (match(TokenType::NE)) {
            op = BinaryOp::Ne;
        } else {
            break;
        }
        std::unique_ptr<Expr> rhs = parse_add_expr();
        auto node = std::make_unique<BinaryExpr>(
            SourceLocation{loc.line, loc.column}, op, std::move(expr), std::move(rhs));
        set_int_type(*node);
        expr = std::move(node);
        loc = current_loc();
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parse_add_expr() {
    SourceLocation loc = current_loc();
    std::unique_ptr<Expr> expr = parse_mul_expr();
    while (true) {
        BinaryOp op;
        if (match(TokenType::PLUS)) {
            op = BinaryOp::Add;
        } else if (match(TokenType::MINUS)) {
            op = BinaryOp::Sub;
        } else {
            break;
        }
        std::unique_ptr<Expr> rhs = parse_mul_expr();
        auto node = std::make_unique<BinaryExpr>(
            SourceLocation{loc.line, loc.column}, op, std::move(expr), std::move(rhs));
        set_int_type(*node);
        expr = std::move(node);
        loc = current_loc();
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parse_mul_expr() {
    SourceLocation loc = current_loc();
    std::unique_ptr<Expr> expr = parse_unary_expr();
    while (true) {
        BinaryOp op;
        if (match(TokenType::STAR)) {
            op = BinaryOp::Mul;
        } else if (match(TokenType::SLASH)) {
            op = BinaryOp::Div;
        } else if (match(TokenType::PERCENT)) {
            op = BinaryOp::Mod;
        } else {
            break;
        }
        std::unique_ptr<Expr> rhs = parse_unary_expr();
        auto node = std::make_unique<BinaryExpr>(
            SourceLocation{loc.line, loc.column}, op, std::move(expr), std::move(rhs));
        set_int_type(*node);
        expr = std::move(node);
        loc = current_loc();
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parse_unary_expr() {
    SourceLocation loc = current_loc();
    if (match(TokenType::PLUS)) {
        auto node = std::make_unique<UnaryExpr>(
            SourceLocation{loc.line, loc.column}, UnaryOp::Plus, parse_unary_expr());
        set_int_type(*node);
        return node;
    }
    if (match(TokenType::MINUS)) {
        auto node = std::make_unique<UnaryExpr>(
            SourceLocation{loc.line, loc.column}, UnaryOp::Minus, parse_unary_expr());
        set_int_type(*node);
        return node;
    }
    if (match(TokenType::BANG)) {
        auto node = std::make_unique<UnaryExpr>(
            SourceLocation{loc.line, loc.column}, UnaryOp::Not, parse_unary_expr());
        set_int_type(*node);
        return node;
    }
    return parse_primary_expr();
}

std::unique_ptr<Expr> Parser::parse_primary_expr() {
    SourceLocation loc = current_loc();
    if (match(TokenType::NUMBER)) {
        const Token& token = previous();
        return std::make_unique<IntLiteral>(
            SourceLocation{token.line, token.column}, token.int_value);
    }
    if (match(TokenType::IDENT)) {
        const Token& name = previous();
        if (match(TokenType::LPAREN)) {
            std::vector<std::unique_ptr<Expr>> args;
            if (!check(TokenType::RPAREN)) {
                do {
                    args.push_back(parse_expr());
                } while (match(TokenType::COMMA));
            }
            consume(TokenType::RPAREN, "expected ')' after arguments");
            return std::make_unique<CallExpr>(
                SourceLocation{loc.line, loc.column}, name.lexeme, std::move(args), false);
        }
        return std::make_unique<IdentifierExpr>(
            SourceLocation{name.line, name.column}, name.lexeme);
    }
    if (match(TokenType::LPAREN)) {
        std::unique_ptr<Expr> expr = parse_expr();
        consume(TokenType::RPAREN, "expected ')' after expression");
        return expr;
    }
    report_error("expected expression", current_loc());
    return std::make_unique<IntLiteral>(SourceLocation{loc.line, loc.column}, 0);
}
