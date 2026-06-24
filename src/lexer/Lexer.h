#pragma once

#include "lexer/LexError.h"
#include "lexer/Token.h"

#include <string>
#include <vector>

class Lexer {
public:
    explicit Lexer(std::string source);

    const std::vector<Token>& tokens() const { return tokens_; }
    const std::vector<LexError>& errors() const { return errors_; }
    bool has_errors() const { return !errors_.empty(); }

private:
    bool is_at_end() const;
    char peek() const;
    char peek_next() const;
    char advance();
    bool match(char expected);

    void skip_whitespace_and_comments();
    void scan_token();

    Token make_token(TokenType type, std::string lexeme, int64_t int_value = 0) const;
    void report_error(const std::string& message, LexLocation loc);

    TokenType identifier_type(const std::string& text) const;
    bool allows_unary_minus_before_number() const;
    void number(bool has_leading_minus);
    void identifier();
    void block_comment();

    std::string source_;
    size_t current_ = 0;
    int line_ = 1;
    int column_ = 1;
    int token_start_line_ = 1;
    int token_start_column_ = 1;
    std::vector<Token> tokens_;
    std::vector<LexError> errors_;
};
