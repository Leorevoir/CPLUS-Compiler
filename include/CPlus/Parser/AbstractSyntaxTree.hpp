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

        void _synchronize();

        /** @brief parsing */
        std::unique_ptr<ast::Program> _parse_program();
        ast::TypePtr _parse_type();

        /** @brief statements */
        ast::StatementPtr _parse_declaration();
        ast::StatementPtr _parse_function_declaration();
        ast::StatementPtr _parse_variable_declaration(bool is_const);
        ast::StatementPtr _parse_statement();
        ast::StatementPtr _parse_block_statement();
        ast::StatementPtr _parse_if_statement();
        ast::StatementPtr _parse_for_statement();
        ast::StatementPtr _parse_foreach_statement();
        ast::StatementPtr _parse_case_statement();
        ast::StatementPtr _parse_return_statement();
        ast::StatementPtr _parse_expression_statement();

        /** @brief expressions */
        ast::ExpressionPtr _parse_expression();
        ast::ExpressionPtr _parse_logical_or();
        ast::ExpressionPtr _parse_logical_and();
        ast::ExpressionPtr _parse_equality();
        ast::ExpressionPtr _parse_comparison();
        ast::ExpressionPtr _parse_term();
        ast::ExpressionPtr _parse_factor();
        ast::ExpressionPtr _parse_unary();
        ast::ExpressionPtr _parse_call();
        ast::ExpressionPtr _finish_call(ast::ExpressionPtr callee);
        ast::ExpressionPtr _parse_primary();
};

}// namespace cplus
