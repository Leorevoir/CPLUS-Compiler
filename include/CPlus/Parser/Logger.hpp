#pragma once

#include <CPlus/Parser/Types.hpp>

#include <iostream>

namespace cplus::ast {

class ASTLogger : public ASTVisitor
{
    public:
        explicit ASTLogger(std::ostream &out = std::cout);

        void show(ASTNode &node);

    private:
        std::ostream &_out;
        i32 _indent = 0;

        /** @brief expressions **/
        void visit(LiteralExpression &node) override;
        void visit(IdentifierExpression &node) override;
        void visit(BinaryExpression &node) override;
        void visit(UnaryExpression &node) override;
        void visit(CallExpression &node) override;
        void visit(AssignmentExpression &node) override;

        /** @brief statements **/
        void visit(ExpressionStatement &node) override;
        void visit(BlockStatement &node) override;
        void visit(VariableDeclaration &node) override;
        void visit(ReturnStatement &node) override;
        void visit(IfStatement &node) override;
        void visit(ForStatement &node) override;
        void visit(ForeachStatement &node) override;
        void visit(CaseStatement &node) override;
        void visit(FunctionDeclaration &node) override;
        void visit(Program &node) override;

        void _show_indent(const std::string &text);
        void _push();
        void _pop();
};

}// namespace cplus::ast
