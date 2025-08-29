#pragma once

#include <CPlus/Compiler/Interface.hpp>
#include <CPlus/Parser/Token.hpp>
#include <CPlus/Parser/Types.hpp>

namespace cplus {

namespace ast {

class AbstractSyntaxTree : public CompilerPass<std::vector<lx::Token>, std::unique_ptr<Module>>
{
    public:
        constexpr AbstractSyntaxTree() = default;
        constexpr ~AbstractSyntaxTree() = default;

        std::unique_ptr<Module> run(const std::vector<lx::Token> &tokens) override;

    private:
        std::vector<lx::Token> _tokens;
        cstr _module;
        u64 _current = 0;

        /** @brief helpers */
        bool _is_at_end() const;
        const lx::Token &_peek() const;
        const lx::Token &_previous() const;
        const lx::Token &_advance();
        const lx::Token &_consume(lx::TokenKind kind, const std::string &message);
        bool _check(lx::TokenKind kind) const;
        bool _check(lx::TokenKind kind, u64 offset) const;
        bool _match(const std::initializer_list<lx::TokenKind> &kinds);

        void _synchronize();

        /** @brief parsing */
        std::unique_ptr<Module> _parse_module();
        TypePtr _parse_type();

        /** @brief statements */
        StatementPtr _parse_declaration();
        StatementPtr _parse_function_declaration();
        StatementPtr _parse_variable_declaration(bool is_const, bool semi_colon);
        StatementPtr _parse_statement();
        StatementPtr _parse_block_statement();
        StatementPtr _parse_if_statement();
        StatementPtr _parse_for_statement();
        StatementPtr _parse_foreach_statement();
        StatementPtr _parse_case_statement();
        StatementPtr _parse_return_statement();
        StatementPtr _parse_expression_statement();

        /** @brief expressions */
        ExpressionPtr _parse_expression();
        ExpressionPtr _parse_logical_or();
        ExpressionPtr _parse_logical_and();
        ExpressionPtr _parse_equality();
        ExpressionPtr _parse_comparison();
        ExpressionPtr _parse_term();
        ExpressionPtr _parse_factor();
        ExpressionPtr _parse_unary();
        ExpressionPtr _parse_call();
        ExpressionPtr _finish_call(ExpressionPtr callee);
        ExpressionPtr _parse_primary();
};

}// namespace ast

}// namespace cplus
