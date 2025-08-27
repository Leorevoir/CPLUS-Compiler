#include <CPlus/Parser/AbstractSyntaxTree.hpp>

/**
* public
*/

std::unique_ptr<cplus::ast::Program> cplus::AbstractSyntaxTree::run(const std::vector<Token> &tokens)
{
    _tokens = tokens;
    _current = 0;

    return _parse_program();
}

/**
 * helpers
 */

bool cplus::AbstractSyntaxTree::_is_at_end() const
{
    return _current >= _tokens.size() || _peek().kind == TokenKind::TOKEN_EOF;
}

const cplus::Token &cplus::AbstractSyntaxTree::_peek() const
{
    if (_is_at_end()) {
        static constexpr Token eof_token{TokenKind::TOKEN_EOF, "", 0, 0};
        return eof_token;
    }
    return _tokens[_current];
}

const cplus::Token &cplus::AbstractSyntaxTree::_previous() const
{
    if (_current == 0) {
        static constexpr Token start_token{TokenKind::TOKEN_EOF, "", 0, 0};
        return start_token;
    }
    return _tokens[_current - 1];
}

const cplus::Token &cplus::AbstractSyntaxTree::_advance()
{
    if (!_is_at_end()) {
        ++_current;
    }
    return _previous();
}

const cplus::Token &cplus::AbstractSyntaxTree::_consume(TokenKind kind, const std::string &message)
{
    if (_check(kind)) {
        return _advance();
    }

    const auto &current = _peek();
    throw exception::Error("AbstractSyntaxTree::_consume", message, " at ", std::to_string(current.line), ":",
        std::to_string(current.column));
}

bool cplus::AbstractSyntaxTree::_check(TokenKind kind) const
{
    if (_is_at_end()) {
        return false;
    }
    return _peek().kind == kind;
}

bool cplus::AbstractSyntaxTree::_match(const std::initializer_list<TokenKind> &kinds)
{
    for (const auto &kind : kinds) {
        if (_check(kind)) {
            _advance();
            return true;
        }
    }
    return false;
}

/**
* private
*/

std::unique_ptr<cplus::ast::Program> cplus::AbstractSyntaxTree::_parse_program()
{
    auto program = ast::make<ast::Program>();

    return program;
}
