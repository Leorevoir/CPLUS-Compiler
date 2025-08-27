#pragma once

#include <CPlus/Compiler/Interface.hpp>
#include <CPlus/Parser/Token.hpp>
#include <CPlus/Parser/Types.hpp>

namespace cplus {

class AbstractSyntaxTree : public CompilerPass<std::vector<Token>, std::unique_ptr<ast::Program>>
{
    public:
        constexpr AbstractSyntaxTree() = default;
        constexpr ~AbstractSyntaxTree() = default;

        std::unique_ptr<ast::Program> run(const std::vector<Token> &tokens) override;

    private:
        std::vector<Token> _tokens;
        u64 _current = 0;

        /** @brief helpers */
        bool _is_at_end() const;
        const Token &_peek() const;
        const Token &_previous() const;
        const Token &_advance();
        const Token &_consume(TokenKind kind, const std::string &message);
        bool _check(TokenKind kind) const;
        bool _match(const std::initializer_list<TokenKind> &kinds);

        /** @brief parsing */
        std::unique_ptr<ast::Program> _parse_program();
};

#undef CPLUS_AST_ACCEPT_VISITOR

}// namespace cplus
