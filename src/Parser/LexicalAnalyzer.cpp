#include "CPlus/Arguments.hpp"
#include "CPlus/Error.hpp"
#include "CPlus/Logger.hpp"
#include <CPlus/Parser/LexicalAnalyzer.hpp>
#include <cctype>
#include <unordered_map>

// clang-format off
static const std::unordered_map<std::string_view, cplus::lx::TokenKind> keywords = {
    {"def", cplus::lx::TokenKind::TOKEN_DEF},
    {"const", cplus::lx::TokenKind::TOKEN_CONST},
    {"return", cplus::lx::TokenKind::TOKEN_RETURN},
    {"struct", cplus::lx::TokenKind::TOKEN_STRUCT},

    {"if", cplus::lx::TokenKind::TOKEN_IF},
    {"elsif", cplus::lx::TokenKind::TOKEN_ELSIF},
    {"else", cplus::lx::TokenKind::TOKEN_ELSE},

    {"for", cplus::lx::TokenKind::TOKEN_FOR},
    {"foreach", cplus::lx::TokenKind::TOKEN_FOREACH},
    {"while", cplus::lx::TokenKind::TOKEN_WHILE},
    {"in", cplus::lx::TokenKind::TOKEN_IN},

    {"case", cplus::lx::TokenKind::TOKEN_CASE},
    {"when", cplus::lx::TokenKind::TOKEN_WHEN},
    {"default", cplus::lx::TokenKind::TOKEN_DEFAULT},
};
// clang-format on

/**
 * public
 */

std::vector<cplus::lx::Token> cplus::lx::LexicalAnalyzer::run(const FileContent &source)
{
    _tokens.clear();
    _source = std::move(source.content);
    _module = source.file.c_str();
    _position = 0;
    _line = 1;
    _column = 1;

    if (cplus_flags & FLAG_DEBUG) {
        logger::info("LexicalAnalyzer::run ", "Lexical analyzing module: ", _module);
    }

    _add_token(TokenKind::TOKEN_MODULE, std::move(source.file));
    _tokens.back().line = 0;
    _tokens.back().column = 0;
    while (!_is_at_end()) {
        _scan_token();
    }
    _add_token(TokenKind::TOKEN_EOF, "");

    if (cplus_flags & FLAG_SHOW_TOKENS) {
        for (const auto &token : _tokens) {
            logger::info("  ", token);
        }
    }

    return _tokens;
}

/**
 * private
 */

void cplus::lx::LexicalAnalyzer::_scan_token()
{
    const char c = _advance();

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
            _add_token(TokenKind::TOKEN_OPEN_PAREN, "(");
            break;
        case ')':
            _add_token(TokenKind::TOKEN_CLOSE_PAREN, ")");
            break;
        case '{':
            _add_token(TokenKind::TOKEN_OPEN_BRACE, "{");
            break;
        case '}':
            _add_token(TokenKind::TOKEN_CLOSE_BRACE, "}");
            break;
        case '[':
            _add_token(TokenKind::TOKEN_OPEN_BRACKET, "[");
            break;
        case ']':
            _add_token(TokenKind::TOKEN_CLOSE_BRACKET, "]");
            break;
        case '.':
            _add_token(TokenKind::TOKEN_DOT, ".");
            break;
        case ',':
            _add_token(TokenKind::TOKEN_COMMA, ",");
            break;
        case ':':
            _add_token(TokenKind::TOKEN_COLON, ":");
            break;
        case ';':
            _add_token(TokenKind::TOKEN_SEMICOLON, ";");
            break;
        case '+':
            if (_match('+')) {
                _add_token(TokenKind::TOKEN_INC, "++");
            } else {
                _add_token(TokenKind::TOKEN_PLUS, "+");
            }
            break;
        case '*':
            _add_token(TokenKind::TOKEN_ASTERISK, "*");
            break;
        case '%':
            _add_token(TokenKind::TOKEN_MODULO, "%");
            break;
        case '^':
            _add_token(TokenKind::TOKEN_XOR, "^");
            break;
        case '~':
            _add_token(TokenKind::TOKEN_NOT, "~");
            break;
        case '"':
            _scan_string();
            break;

        case '\'':
            _scan_character();
            break;
        case '-':
            if (_match('>')) {
                _add_token(TokenKind::TOKEN_ARROW, "->");
            } else if (_match('-')) {
                _add_token(TokenKind::TOKEN_DEC, "--");
            } else {
                _add_token(TokenKind::TOKEN_MINUS, "-");
            }
            break;

        case '!':
            if (_match('=')) {
                _add_token(TokenKind::TOKEN_NEQ, "!=");
            } else {
                _add_token(TokenKind::TOKEN_CMP_NOT, "!");
            }
            break;

        case '=':
            if (_match('=')) {
                _add_token(TokenKind::TOKEN_EQ, "==");
            } else {
                _add_token(TokenKind::TOKEN_ASSIGN, "=");
            }
            break;

        case '<':
            if (_match('=')) {
                _add_token(TokenKind::TOKEN_LTE, "<=");
            } else {
                _add_token(TokenKind::TOKEN_LT, "<");
            }
            break;

        case '>':
            if (_match('=')) {
                _add_token(TokenKind::TOKEN_GTE, ">=");
            } else {
                _add_token(TokenKind::TOKEN_GT, ">");
            }
            break;

        case '&':
            if (_match('&')) {
                _add_token(TokenKind::TOKEN_CMP_AND, "&&");
            } else {
                _add_token(TokenKind::TOKEN_AND, "&");
            }
            break;

        case '|':
            if (_match('|')) {
                _add_token(TokenKind::TOKEN_CMP_OR, "||");
            } else {
                _add_token(TokenKind::TOKEN_OR, "|");
            }
            break;

        case '/':
            if (_match('/')) {
                _skip_line_comment();
            } else if (_match('*')) {
                _skip_block_comment();
            } else {
                _add_token(TokenKind::TOKEN_SLASH, "/");
            }
            break;

        default:
            if (std::isdigit(c)) {
                _scan_number();
            } else if (std::isalpha(c) || c == '_') {
                _scan_identifier();
            } else {
                throw exception::Error("LexicalAnalyzer", "Unexpected character in module: ", _module, " at ", _line, ":", _column);
            }
            break;
    }
}

char cplus::lx::LexicalAnalyzer::_advance()
{
    if (_is_at_end()) {
        return '\0';
    }
    ++_column;
    return _source[_position++];
}

bool cplus::lx::LexicalAnalyzer::_match(char expected)
{
    if (_is_at_end()) {
        return false;
    }
    if (_source[_position] != expected) {
        return false;
    }
    ++_position;
    ++_column;
    return true;
}

char cplus::lx::LexicalAnalyzer::_peek() const
{
    if (_is_at_end()) {
        return '\0';
    }
    return _source[_position];
}

char cplus::lx::LexicalAnalyzer::_peek_next() const
{
    if (_position + 1 >= _source.length()) {
        return '\0';
    }
    return _source[_position + 1];
}

bool cplus::lx::LexicalAnalyzer::_is_at_end() const
{
    return _position >= _source.length();
}

void cplus::lx::LexicalAnalyzer::_skip_line_comment()
{
    while (_peek() != '\n' && !_is_at_end()) {
        _advance();
    }
}

void cplus::lx::LexicalAnalyzer::_skip_block_comment()
{
    while (!_is_at_end()) {
        if (_peek() == '*' && _peek_next() == '/') {
            _advance();//<< consume *
            _advance();//<< consume /
            break;
        }

        if (_peek() == '\n') {
            ++_line;
            _column = 0;
        }

        _advance();
    }
}

void cplus::lx::LexicalAnalyzer::_scan_number()
{
    const size_t start = _position - 1;
    bool is_float = false;

    while (std::isdigit(_peek())) {
        _advance();
    }

    if (_peek() == '.' && std::isdigit(_peek_next())) {
        is_float = true;
        _advance();//<< consume .

        while (std::isdigit(_peek())) {
            _advance();
        }
    }

    const std::string_view lexeme(_source.data() + start, _position - start);

    _add_token(is_float ? TokenKind::TOKEN_FLOAT : TokenKind::TOKEN_INTEGER, lexeme);
}

void cplus::lx::LexicalAnalyzer::_scan_identifier()
{
    const size_t start = _position - 1;

    while (std::isalnum(_peek()) || _peek() == '_') {
        _advance();
    }

    const std::string_view lexeme(_source.data() + start, _position - start);

    const auto it = keywords.find(lexeme);
    const TokenKind kind = (it != keywords.end()) ? it->second : TokenKind::TOKEN_IDENTIFIER;

    _add_token(kind, lexeme);
}

void cplus::lx::LexicalAnalyzer::_scan_string()
{
    const size_t start = _position - 1;
    const u64 start_line = _line;
    const u64 start_column = _column - 1;

    while (_peek() != '"' && !_is_at_end()) {
        if (_peek() == '\n') {
            ++_line;
            _column = 0;
        }

        if (_peek() == '\\') {
            _advance();//<< consume backslash
            if (!_is_at_end()) {
                _advance();//<< consume escaped character
            }
        } else {
            _advance();
        }
    }

    if (_is_at_end()) {
        throw exception::Error("LexicalAnalyzer", "Unterminated string in module: ", _module, " at ", start_line, ":", start_column);
    }

    _advance();

    const std::string_view lexeme(_source.data() + start, _position - start);
    _add_token(TokenKind::TOKEN_STRING, lexeme);
}

void cplus::lx::LexicalAnalyzer::_scan_character()
{
    const size_t start = _position - 1;
    const u64 start_line = _line;
    const u64 start_column = _column - 1;

    if (_peek() == '\'') {
        _advance();//<< consume closing quote
        throw exception::Error("LexicalAnalyzer", "Empty character literal in module: ", _module, " at ", start_line, ":", start_column);
    }

    if (_peek() == '\\') {
        _advance();//<< consume backslash
        if (!_is_at_end()) {
            _advance();//<< consume escaped character
        }
    } else {
        _advance();//<< consume the character
    }

    if (_peek() != '\'' || _is_at_end()) {
        throw exception::Error("LexicalAnalyzer", "Unterminated character literal in module: ", _module, " at ", start_line, ":",
            start_column);
    }

    _advance();//<< consume closing quote

    const std::string_view lexeme(_source.data() + start, _position - start);
    _add_token(TokenKind::TOKEN_CHARACTER, lexeme);
}

void cplus::lx::LexicalAnalyzer::_add_token(TokenKind kind, std::string_view lexeme)
{
    _tokens.push_back({.kind = kind, .lexeme = lexeme, .line = _line, .column = _column - lexeme.length()});
}
