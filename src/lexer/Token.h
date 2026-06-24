#pragma once

#include <cstdint>
#include <string>

enum class TokenType {
    // 关键字
    KW_CONST,
    KW_INT,
    KW_VOID,
    KW_IF,
    KW_ELSE,
    KW_WHILE,
    KW_BREAK,
    KW_CONTINUE,
    KW_RETURN,

    // 标识符与字面量
    IDENT,
    NUMBER,

    // 运算符
    PLUS,
    MINUS,
    STAR,
    SLASH,
    PERCENT,
    LT,
    GT,
    LE,
    GE,
    EQ,
    NE,
    AND_AND,
    OR_OR,
    BANG,
    ASSIGN,

    // 分隔符
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    COMMA,
    SEMICOLON,

    END_OF_FILE,
};

struct Token {
    TokenType type = TokenType::END_OF_FILE;
    std::string lexeme;
    int line = 1;
    int column = 1;
    int64_t int_value = 0;

    Token() = default;

    Token(TokenType type, std::string lexeme, int line, int column, int64_t int_value = 0)
        : type(type), lexeme(std::move(lexeme)), line(line), column(column), int_value(int_value) {}

    bool is(TokenType t) const { return type == t; }
    bool is_one_of(TokenType a, TokenType b) const { return type == a || type == b; }
    bool is_one_of(TokenType a, TokenType b, TokenType c) const {
        return type == a || type == b || type == c;
    }
};

const char* token_type_name(TokenType type);
