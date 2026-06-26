#include "lexer/Lexer.h"

#include "lexer/LexError.h"

#include <cctype>

namespace {

bool is_ident_start(char c) {
    return c == '_' || std::isalpha(static_cast<unsigned char>(c)) != 0;
}

bool is_ident_continue(char c) {
    return is_ident_start(c) || std::isdigit(static_cast<unsigned char>(c)) != 0;
}

} // namespace

Lexer::Lexer(std::string source) : source_(std::move(source)) {
    while (!is_at_end()) {
        token_start_line_ = line_;
        token_start_column_ = column_;
        skip_whitespace_and_comments();
        if (is_at_end()) {
            break;
        }
        token_start_line_ = line_;
        token_start_column_ = column_;
        scan_token();
    }
    tokens_.emplace_back(TokenType::END_OF_FILE, "", line_, column_);
}

bool Lexer::is_at_end() const {
    return current_ >= source_.size();
}

char Lexer::peek() const {
    if (is_at_end()) {
        return '\0';
    }
    return source_[current_];
}

char Lexer::peek_next() const {
    if (current_ + 1 >= source_.size()) {
        return '\0';
    }
    return source_[current_ + 1];
}

char Lexer::advance() {
    char c = source_[current_++];
    if (c == '\n') {
        ++line_;
        column_ = 1;
    } else {
        ++column_;
    }
    return c;
}

bool Lexer::match(char expected) {
    if (is_at_end() || source_[current_] != expected) {
        return false;
    }
    advance();
    return true;
}

void Lexer::skip_whitespace_and_comments() {
    for (;;) {
        char c = peek();
        switch (c) {
        case ' ':
        case '\r':
        case '\t':
        case '\n':
            advance();
            break;
        case '/':
            if (peek_next() == '/') {
                while (!is_at_end() && peek() != '\n') {
                    advance();
                }
            } else if (peek_next() == '*') {
                advance();
                advance();
                block_comment();
            } else {
                return;
            }
            break;
        default:
            return;
        }
    }
}

void Lexer::block_comment() {
    while (!is_at_end()) {
        if (peek() == '*' && peek_next() == '/') {
            advance();
            advance();
            return;
        }
        advance();
    }
    report_error("unterminated block comment", {line_, column_});
}

void Lexer::scan_token() {
    char c = advance();
    switch (c) {
    case '(':
        tokens_.push_back(make_token(TokenType::LPAREN, "("));
        return;
    case ')':
        tokens_.push_back(make_token(TokenType::RPAREN, ")"));
        return;
    case '{':
        tokens_.push_back(make_token(TokenType::LBRACE, "{"));
        return;
    case '}':
        tokens_.push_back(make_token(TokenType::RBRACE, "}"));
        return;
    case ',':
        tokens_.push_back(make_token(TokenType::COMMA, ","));
        return;
    case ';':
        tokens_.push_back(make_token(TokenType::SEMICOLON, ";"));
        return;
    case '+':
        tokens_.push_back(make_token(TokenType::PLUS, "+"));
        return;
    case '*':
        tokens_.push_back(make_token(TokenType::STAR, "*"));
        return;
    case '/':
        tokens_.push_back(make_token(TokenType::SLASH, "/"));
        return;
    case '%':
        tokens_.push_back(make_token(TokenType::PERCENT, "%"));
        return;
    case '-':
        if (std::isdigit(static_cast<unsigned char>(peek())) != 0 &&
            allows_unary_minus_before_number()) {
            number(true);
        } else {
            tokens_.push_back(make_token(TokenType::MINUS, "-"));
        }
        return;
    case '!':
        if (match('=')) {
            tokens_.push_back(make_token(TokenType::NE, "!="));
        } else {
            tokens_.push_back(make_token(TokenType::BANG, "!"));
        }
        return;
    case '<':
        if (match('=')) {
            tokens_.push_back(make_token(TokenType::LE, "<="));
        } else {
            tokens_.push_back(make_token(TokenType::LT, "<"));
        }
        return;
    case '>':
        if (match('=')) {
            tokens_.push_back(make_token(TokenType::GE, ">="));
        } else {
            tokens_.push_back(make_token(TokenType::GT, ">"));
        }
        return;
    case '=':
        if (match('=')) {
            tokens_.push_back(make_token(TokenType::EQ, "=="));
        } else {
            tokens_.push_back(make_token(TokenType::ASSIGN, "="));
        }
        return;
    case '&':
        if (match('&')) {
            tokens_.push_back(make_token(TokenType::AND_AND, "&&"));
        } else {
            report_error(
                "invalid operator '&' (use '&&' for logical and)",
                {token_start_line_, token_start_column_});
        }
        return;
    case '|':
        if (match('|')) {
            tokens_.push_back(make_token(TokenType::OR_OR, "||"));
        } else {
            report_error(
                "invalid operator '|' (use '||' for logical or)",
                {token_start_line_, token_start_column_});
        }
        return;
    default:
        break;
    }

    if (is_ident_start(c)) {
        current_--;
        if (column_ > 1) {
            --column_;
        }
        identifier();
        return;
    }
    if (std::isdigit(static_cast<unsigned char>(c)) != 0) {
        current_--;
        if (column_ > 1) {
            --column_;
        }
        number(false);
        return;
    }

    report_error(
        "unexpected character '" + std::string(1, c) + "'",
        {token_start_line_, token_start_column_});
}

Token Lexer::make_token(TokenType type, std::string lexeme, int64_t int_value) const {
    return Token(type, std::move(lexeme), token_start_line_, token_start_column_, int_value);
}

void Lexer::report_error(const std::string& message, LexLocation loc) {
    errors_.emplace_back(message, loc);
}

void Lexer::identifier() {
    size_t start = current_;
    while (is_ident_continue(peek())) {
        advance();
    }

    const std::string text = source_.substr(start, current_ - start);
    tokens_.push_back(make_token(identifier_type(text), text));
}

bool Lexer::allows_unary_minus_before_number() const {
    if (tokens_.empty()) {
        return true;
    }

    switch (tokens_.back().type) {
    case TokenType::IDENT:
    case TokenType::NUMBER:
    case TokenType::RPAREN:
        return false;
    default:
        return true;
    }
}

TokenType Lexer::identifier_type(const std::string& text) const {
    if (text == "const") return TokenType::KW_CONST;
    if (text == "int") return TokenType::KW_INT;
    if (text == "void") return TokenType::KW_VOID;
    if (text == "if") return TokenType::KW_IF;
    if (text == "else") return TokenType::KW_ELSE;
    if (text == "while") return TokenType::KW_WHILE;
    if (text == "break") return TokenType::KW_BREAK;
    if (text == "continue") return TokenType::KW_CONTINUE;
    if (text == "return") return TokenType::KW_RETURN;
    return TokenType::IDENT;
}

void Lexer::number(bool has_leading_minus) {
    const size_t start = has_leading_minus ? current_ - 1 : current_;

    if (peek() == '0') {
        advance();
        if (std::isdigit(static_cast<unsigned char>(peek())) != 0) {
            while (std::isdigit(static_cast<unsigned char>(peek())) != 0) {
                advance();
            }
            if (is_ident_start(peek())) {
                while (is_ident_continue(peek())) {
                    advance();
                }
            }
            const std::string bad = source_.substr(start, current_ - start);
            report_error(
                "invalid number '" + bad + "': leading zeros are not allowed",
                {token_start_line_, token_start_column_});
            return;
        }
    } else {
        if (std::isdigit(static_cast<unsigned char>(peek())) == 0) {
            report_error(
                "invalid number",
                {token_start_line_, token_start_column_});
            return;
        }
        while (std::isdigit(static_cast<unsigned char>(peek())) != 0) {
            advance();
        }
    }

    if (is_ident_start(peek())) {
        while (is_ident_continue(peek())) {
            advance();
        }
        const std::string bad = source_.substr(start, current_ - start);
        report_error(
            "invalid token '" + bad + "': identifier cannot start with a digit",
            {token_start_line_, token_start_column_});
        return;
    }

    const std::string text = source_.substr(start, current_ - start);
    const int64_t value = std::stoll(text);
    tokens_.push_back(make_token(TokenType::NUMBER, text, value));
}
