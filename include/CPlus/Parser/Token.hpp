#pragma once

#include "CPlus/Logger.hpp"
#include <CPlus/Types.hpp>

#include <fstream>
#include <string_view>

namespace cplus {

enum class TokenKind {
    TOKEN_DEF,
    TOKEN_CONST,
    TOKEN_RETURN,
    TOKEN_STRUCT,
    TOKEN_IDENTIFIER,

    TOKEN_CHARACTER,
    TOKEN_STRING,
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
    TOKEN_DEFAULT,

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

static inline constexpr cplus::cstr to_string(const TokenKind kind)
{
    switch (kind) {
        case TokenKind::TOKEN_DEF:
            return "DEF";
        case TokenKind::TOKEN_CONST:
            return "CONST";
        case TokenKind::TOKEN_RETURN:
            return "RETURN";
        case TokenKind::TOKEN_STRUCT:
            return "STRUCT";
        case TokenKind::TOKEN_IDENTIFIER:
            return "IDENTIFIER";

        case TokenKind::TOKEN_CHARACTER:
            return "CHARACTER";
        case TokenKind::TOKEN_STRING:
            return "STRING";
        case TokenKind::TOKEN_INTEGER:
            return "INTEGER";
        case TokenKind::TOKEN_FLOAT:
            return "FLOAT";

        case TokenKind::TOKEN_IF:
            return "IF";
        case TokenKind::TOKEN_ELSIF:
            return "ELSIF";
        case TokenKind::TOKEN_ELSE:
            return "ELSE";

        case TokenKind::TOKEN_FOR:
            return "FOR";
        case TokenKind::TOKEN_FOREACH:
            return "FOREACH";
        case TokenKind::TOKEN_WHILE:
            return "WHILE";

        case TokenKind::TOKEN_CASE:
            return "CASE";
        case TokenKind::TOKEN_WHEN:
            return "WHEN";
        case TokenKind::TOKEN_DEFAULT:
            return "DEFAULT";

        case TokenKind::TOKEN_OPEN_PAREN:
            return "OPEN_PAREN";
        case TokenKind::TOKEN_CLOSE_PAREN:
            return "CLOSE_PAREN";
        case TokenKind::TOKEN_OPEN_BRACE:
            return "OPEN_BRACE";
        case TokenKind::TOKEN_CLOSE_BRACE:
            return "CLOSE_BRACE";
        case TokenKind::TOKEN_OPEN_BRACKET:
            return "OPEN_BRACKET";
        case TokenKind::TOKEN_CLOSE_BRACKET:
            return "CLOSE_BRACKET";

        case TokenKind::TOKEN_DOT:
            return "DOT";
        case TokenKind::TOKEN_COMMA:
            return "COMMA";
        case TokenKind::TOKEN_COLON:
            return "COLON";
        case TokenKind::TOKEN_SEMICOLON:
            return "SEMICOLON";
        case TokenKind::TOKEN_ARROW:
            return "ARROW";

        case TokenKind::TOKEN_EQ:
            return "EQ";
        case TokenKind::TOKEN_NEQ:
            return "NEQ";
        case TokenKind::TOKEN_LT:
            return "LT";
        case TokenKind::TOKEN_LTE:
            return "LTE";
        case TokenKind::TOKEN_GT:
            return "GT";
        case TokenKind::TOKEN_GTE:
            return "GTE";

        case TokenKind::TOKEN_PLUS:
            return "PLUS";
        case TokenKind::TOKEN_MINUS:
            return "MINUS";
        case TokenKind::TOKEN_ASTERISK:
            return "ASTERISK";
        case TokenKind::TOKEN_SLASH:
            return "SLASH";
        case TokenKind::TOKEN_MODULO:
            return "MODULO";

        case TokenKind::TOKEN_CMP_AND:
            return "CMP_AND";
        case TokenKind::TOKEN_CMP_OR:
            return "CMP_OR";
        case TokenKind::TOKEN_CMP_NOT:
            return "CMP_NOT";
        case TokenKind::TOKEN_XOR:
            return "XOR";

        case TokenKind::TOKEN_ASSIGN:
            return "ASSIGN";

        case TokenKind::TOKEN_EOF:
            return "EOF";

        default:
            return "UNKNOWN";
    }
}

static inline std::ostream &operator<<(std::ostream &os, const Token &token)
{
    return os << logger::CPLUS_MAGENTA << to_string(token.kind) << logger::CPLUS_RESET << "('" << logger::CPLUS_YELLOW << token.lexeme
              << logger::CPLUS_RESET << "') at " << token.line << ":" << token.column;
}

}// namespace cplus
