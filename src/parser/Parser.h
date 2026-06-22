#pragma once

#include "ast/CompUnit.h"
#include "lexer/Token.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& message, SourceLocation loc)
        : std::runtime_error(format_message(message, loc)), loc_(loc) {}

    const SourceLocation& location() const { return loc_; }

private:
    static std::string format_message(const std::string& message, SourceLocation loc) {
        return message + " at line " + std::to_string(loc.line) + ", column " +
               std::to_string(loc.column);
    }

    SourceLocation loc_;
};

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);

    std::unique_ptr<CompUnit> parse();
    const std::vector<ParseError>& errors() const { return errors_; }
    bool has_errors() const { return !errors_.empty(); }

private:
    bool is_at_end() const;
    const Token& peek() const;
    const Token& previous() const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    const Token& consume(TokenType type, const char* message);

    void report_error(const char* message, SourceLocation loc);
    void recover_to(TokenType type);
    void recover_semicolon();
    void synchronize_top_level();

    SourceLocation current_loc() const;
    SourceLocation previous_loc() const;

    std::unique_ptr<CompUnit> parse_comp_unit();
    TopLevelItem parse_top_level_item();
    std::unique_ptr<VarDecl> parse_var_decl();
    std::unique_ptr<ConstDecl> parse_const_decl();
    std::unique_ptr<FuncDef> parse_func_def(FuncReturnType return_type);
    std::unique_ptr<Param> parse_param();
    std::unique_ptr<BlockStmt> parse_block();
    std::unique_ptr<Stmt> parse_stmt();
    std::unique_ptr<Stmt> parse_if_stmt();
    std::unique_ptr<Stmt> parse_while_stmt();
    std::unique_ptr<Stmt> parse_return_stmt();

    std::unique_ptr<Expr> parse_expr();
    std::unique_ptr<Expr> parse_lor_expr();
    std::unique_ptr<Expr> parse_land_expr();
    std::unique_ptr<Expr> parse_rel_expr();
    std::unique_ptr<Expr> parse_add_expr();
    std::unique_ptr<Expr> parse_mul_expr();
    std::unique_ptr<Expr> parse_unary_expr();
    std::unique_ptr<Expr> parse_primary_expr();

    TopLevelItem make_error_var_decl();

    const std::vector<Token>& tokens_;
    size_t current_ = 0;
    std::vector<ParseError> errors_;
};
