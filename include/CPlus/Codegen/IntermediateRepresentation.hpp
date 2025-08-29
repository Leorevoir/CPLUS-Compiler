#pragma once

#include <CPlus/Analysis/SymbolTable.hpp>
#include <CPlus/Compiler/Interface.hpp>
#include <CPlus/Parser/Types.hpp>

namespace cplus::ir {

/**
 * @brief IntermediateRepresentation
 * @details Converts AST + symbol table into IR
 * @input st::ASTScope (AST + symbol table)
 * @output std::string (IR as text)
 */
class IntermediateRepresentation : public CompilerPass<std::unique_ptr<ast::Module>, std::string>, public ast::ASTVisitor
{
    public:
        IntermediateRepresentation() = default;
        ~IntermediateRepresentation() override = default;

        std::string run(const std::unique_ptr<cplus::ast::Module> &scope) override;

    private:
        std::string _output;
        std::string _current_function;
        std::string _last_value;

        u64 _temp_counter = 0;
        u64 _label_counter = 0;

        void emit(const std::string &s);

        std::string new_temp(const std::string &hint);
        std::string new_label(const std::string &hint);

        void visit(ast::LiteralExpression &node) override;
        void visit(ast::IdentifierExpression &node) override;
        void visit(ast::BinaryExpression &node) override;
        void visit(ast::UnaryExpression &node) override;
        void visit(ast::CallExpression &node) override;
        void visit(ast::AssignmentExpression &node) override;
        void visit(ast::ExpressionStatement &node) override;
        void visit(ast::BlockStatement &node) override;
        void visit(ast::VariableDeclaration &node) override;
        void visit(ast::ReturnStatement &node) override;
        void visit(ast::IfStatement &node) override;
        void visit(ast::ForStatement &node) override;
        void visit(ast::ForeachStatement &node) override;
        void visit(ast::CaseStatement &node) override;
        void visit(ast::FunctionDeclaration &node) override;
        void visit(ast::Module &node) override;
};

}// namespace cplus::ir
