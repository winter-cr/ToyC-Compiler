#include "lexer/Token.h"

const char* token_type_name(TokenType type) {
    switch (type) {
    case TokenType::KW_CONST: return "const";
    case TokenType::KW_INT: return "int";
    case TokenType::KW_VOID: return "void";
    case TokenType::KW_IF: return "if";
    case TokenType::KW_ELSE: return "else";
    case TokenType::KW_WHILE: return "while";
    case TokenType::KW_BREAK: return "break";
    case TokenType::KW_CONTINUE: return "continue";
    case TokenType::KW_RETURN: return "return";
    case TokenType::IDENT: return "ident";
    case TokenType::NUMBER: return "number";
    case TokenType::PLUS: return "+";
    case TokenType::MINUS: return "-";
    case TokenType::STAR: return "*";
    case TokenType::SLASH: return "/";
    case TokenType::PERCENT: return "%";
    case TokenType::LT: return "<";
    case TokenType::GT: return ">";
    case TokenType::LE: return "<=";
    case TokenType::GE: return ">=";
    case TokenType::EQ: return "==";
    case TokenType::NE: return "!=";
    case TokenType::AND_AND: return "&&";
    case TokenType::OR_OR: return "||";
    case TokenType::BANG: return "!";
    case TokenType::ASSIGN: return "=";
    case TokenType::LPAREN: return "(";
    case TokenType::RPAREN: return ")";
    case TokenType::LBRACE: return "{";
    case TokenType::RBRACE: return "}";
    case TokenType::COMMA: return ",";
    case TokenType::SEMICOLON: return ";";
    case TokenType::END_OF_FILE: return "EOF";
    }
    return "?";
}
