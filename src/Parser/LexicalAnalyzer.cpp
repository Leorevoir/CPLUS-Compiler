#include "CPlus/Error.hpp"
#include <CPlus/Parser/LexicalAnalyzer.hpp>
#include <cctype>
#include <unordered_map>

// clang-format off
static const std::unordered_map<std::string_view, cplus::TokenKind> keywords = {
    {"def", cplus::TokenKind::TOKEN_DEF},
    {"return", cplus::TokenKind::TOKEN_RETURN},

    {"if", cplus::TokenKind::TOKEN_IF},
    {"elsif", cplus::TokenKind::TOKEN_ELSIF},
    {"else", cplus::TokenKind::TOKEN_ELSE},

    {"for", cplus::TokenKind::TOKEN_FOR},
    {"foreach", cplus::TokenKind::TOKEN_FOREACH},
    {"while", cplus::TokenKind::TOKEN_WHILE},

    {"case", cplus::TokenKind::TOKEN_CASE},
    {"when", cplus::TokenKind::TOKEN_WHEN}
};
// clang-format on

/**
 * public
 */

std::vector<cplus::Token> cplus::LexicalAnalyzer::run(const std::string &source)
{
    _tokens.clear();
    _source = source;
    _position = 0;
    _line = 1;
    _column = 1;

    while (!is_at_end()) {
        scan_token();
    }
    add_token(TokenKind::TOKEN_EOF, "");
    return _tokens;
}

/**
 * private
 */

void cplus::LexicalAnalyzer::scan_token()
{
    const char c = advance();

    switch (c) {
        /** @brief whitespace */
        case ' ':
        case '\r':
        case '\t':
            break;

        case '\n':
            ++_line;
            _column = 1;
            break;

        /** @brief single character tokens */
        case '(':
            add_token(TokenKind::TOKEN_OPEN_PAREN, "(");
            break;
        case ')':
            add_token(TokenKind::TOKEN_CLOSE_PAREN, ")");
            break;
        case '{':
            add_token(TokenKind::TOKEN_OPEN_BRACE, "{");
            break;
        case '}':
            add_token(TokenKind::TOKEN_CLOSE_BRACE, "}");
            break;
        case '[':
            add_token(TokenKind::TOKEN_OPEN_BRACKET, "[");
            break;
        case ']':
            add_token(TokenKind::TOKEN_CLOSE_BRACKET, "]");
            break;
        case '.':
            add_token(TokenKind::TOKEN_DOT, ".");
            break;
        case ',':
            add_token(TokenKind::TOKEN_COMMA, ",");
            break;
        case ':':
            add_token(TokenKind::TOKEN_COLON, ":");
            break;
        case ';':
            add_token(TokenKind::TOKEN_SEMICOLON, ";");
            break;
        case '+':
            add_token(TokenKind::TOKEN_PLUS, "+");
            break;
        case '*':
            add_token(TokenKind::TOKEN_ASTERISK, "*");
            break;
        case '%':
            add_token(TokenKind::TOKEN_MODULO, "%");
            break;
        case '^':
            add_token(TokenKind::TOKEN_XOR, "^");
            break;
        case '~':
            add_token(TokenKind::TOKEN_NOT, "~");
            break;

        case '-':
            if (match('>')) {
                add_token(TokenKind::TOKEN_ARROW, "->");
            } else {
                add_token(TokenKind::TOKEN_MINUS, "-");
            }
            break;

        case '!':
            if (match('=')) {
                add_token(TokenKind::TOKEN_NEQ, "!=");
            } else {
                add_token(TokenKind::TOKEN_CMP_NOT, "!");
            }
            break;

        case '=':
            if (match('=')) {
                add_token(TokenKind::TOKEN_EQ, "==");
            } else {
                add_token(TokenKind::TOKEN_ASSIGN, "=");
            }
            break;

        case '<':
            if (match('=')) {
                add_token(TokenKind::TOKEN_LTE, "<=");
            } else {
                add_token(TokenKind::TOKEN_LT, "<");
            }
            break;

        case '>':
            if (match('=')) {
                add_token(TokenKind::TOKEN_GTE, ">=");
            } else {
                add_token(TokenKind::TOKEN_GT, ">");
            }
            break;

        case '&':
            if (match('&')) {
                add_token(TokenKind::TOKEN_CMP_AND, "&&");
            } else {
                add_token(TokenKind::TOKEN_AND, "&");
            }
            break;

        case '|':
            if (match('|')) {
                add_token(TokenKind::TOKEN_CMP_OR, "||");
            } else {
                add_token(TokenKind::TOKEN_OR, "|");
            }
            break;

        case '/':
            if (match('/')) {
                skip_line_comment();
            } else if (match('*')) {
                skip_block_comment();
            } else {
                add_token(TokenKind::TOKEN_SLASH, "/");
            }
            break;

        default:
            if (std::isdigit(c)) {
                scan_number();
            } else if (std::isalpha(c) || c == '_') {
                scan_identifier();
            } else {
                throw exception::Error("LexicalAnalyzer", "Unexpected character at ", _line, ":", _column);
            }
            break;
    }
}

char cplus::LexicalAnalyzer::advance()
{
    if (is_at_end()) {
        return '\0';
    }
    ++_column;
    return _source[_position++];
}

bool cplus::LexicalAnalyzer::match(char expected)
{
    if (is_at_end()) {
        return false;
    }
    if (_source[_position] != expected) {
        return false;
    }
    ++_position;
    ++_column;
    return true;
}

char cplus::LexicalAnalyzer::peek() const
{
    if (is_at_end()) {
        return '\0';
    }
    return _source[_position];
}

char cplus::LexicalAnalyzer::peek_next() const
{
    if (_position + 1 >= _source.length()) {
        return '\0';
    }
    return _source[_position + 1];
}

bool cplus::LexicalAnalyzer::is_at_end() const
{
    return _position >= _source.length();
}

void cplus::LexicalAnalyzer::skip_line_comment()
{
    while (peek() != '\n' && !is_at_end()) {
        advance();
    }
}

void cplus::LexicalAnalyzer::skip_block_comment()
{
    while (!is_at_end()) {
        if (peek() == '*' && peek_next() == '/') {
            advance();//<< consume *
            advance();//<< consume /
            break;
        }

        if (peek() == '\n') {
            ++_line;
            _column = 0;
        }

        advance();
    }
}

void cplus::LexicalAnalyzer::scan_number()
{
    const size_t start = _position - 1;
    bool is_float = false;

    while (std::isdigit(peek())) {
        advance();
    }

    if (peek() == '.' && std::isdigit(peek_next())) {
        is_float = true;
        advance();//<< consume .

        while (std::isdigit(peek())) {
            advance();
        }
    }

    const std::string_view lexeme(_source.data() + start, _position - start);

    add_token(is_float ? TokenKind::TOKEN_FLOAT : TokenKind::TOKEN_INTEGER, lexeme);
}

void cplus::LexicalAnalyzer::scan_identifier()
{
    const size_t start = _position - 1;

    while (std::isalnum(peek()) || peek() == '_') {
        advance();
    }

    const std::string_view lexeme(_source.data() + start, _position - start);

    const auto it = keywords.find(lexeme);
    const TokenKind kind = (it != keywords.end()) ? it->second : TokenKind::TOKEN_IDENTIFIER;

    add_token(kind, lexeme);
}

void cplus::LexicalAnalyzer::add_token(TokenKind kind, std::string_view lexeme)
{
    _tokens.push_back({.kind = kind, .lexeme = lexeme, .line = _line, .column = _column - lexeme.length()});
}
