#pragma once

#include <CPlus/Types.hpp>

#include <string_view>

namespace cplus {

enum class TokenKind {
    TOKEN_DEF,
    TOKEN_RETURN,
    TOKEN_IDENTIFIER,

    TOKEN_INTEGER,
    TOKEN_FLOAT,

    TOKEN_IF,
    TOKEN_ELSIF,
    TOKEN_ELSE,

    TOKEN_FOR,
    TOKEN_FOREACH,
    TOKEN_WHILE,

    TOKEN_CASE,
    TOKEN_WHEN,

    TOKEN_OPEN_PAREN,
    TOKEN_CLOSE_PAREN,
    TOKEN_OPEN_BRACE,
    TOKEN_CLOSE_BRACE,
    TOKEN_OPEN_BRACKET,
    TOKEN_CLOSE_BRACKET,

    TOKEN_DOT,
    TOKEN_COMMA,
    TOKEN_COLON,
    TOKEN_SEMICOLON,
    TOKEN_ARROW,

    TOKEN_EQ,
    TOKEN_NEQ,
    TOKEN_LT,
    TOKEN_LTE,
    TOKEN_GT,
    TOKEN_GTE,

    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_ASTERISK,
    TOKEN_SLASH,
    TOKEN_MODULO,

    TOKEN_CMP_AND,
    TOKEN_CMP_OR,
    TOKEN_CMP_NOT,

    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,
    TOKEN_XOR,

    TOKEN_ASSIGN,

    TOKEN_EOF,
};

struct Token {
        TokenKind kind;
        std::string_view lexeme;
        u64 line;
        u64 column;
};

}// namespace cplus
