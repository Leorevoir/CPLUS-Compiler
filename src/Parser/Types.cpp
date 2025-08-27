#include <CPlus/Parser/Types.hpp>

void cplus::ast::LiteralExpression::accept(cplus::ast::ASTVisitor &visitor)
{
    visitor.visit(*this);
}

void cplus::ast::IdentifierExpression::accept(cplus::ast::ASTVisitor &visitor)
{
    visitor.visit(*this);
}

void cplus::ast::BinaryExpression::accept(cplus::ast::ASTVisitor &visitor)
{
    visitor.visit(*this);
}

void cplus::ast::UnaryExpression::accept(cplus::ast::ASTVisitor &visitor)
{
    visitor.visit(*this);
}

void cplus::ast::CallExpression::accept(cplus::ast::ASTVisitor &visitor)
{
    visitor.visit(*this);
}

void cplus::ast::AssignmentExpression::accept(cplus::ast::ASTVisitor &visitor)
{
    visitor.visit(*this);
}

void cplus::ast::ExpressionStatement::accept(cplus::ast::ASTVisitor &visitor)
{
    visitor.visit(*this);
}

void cplus::ast::BlockStatement::accept(cplus::ast::ASTVisitor &visitor)
{
    visitor.visit(*this);
}

void cplus::ast::VariableDeclaration::accept(cplus::ast::ASTVisitor &visitor)
{
    visitor.visit(*this);
}

void cplus::ast::ReturnStatement::accept(cplus::ast::ASTVisitor &visitor)
{
    visitor.visit(*this);
}

void cplus::ast::IfStatement::accept(cplus::ast::ASTVisitor &visitor)
{
    visitor.visit(*this);
}

void cplus::ast::ForStatement::accept(cplus::ast::ASTVisitor &visitor)
{
    visitor.visit(*this);
}

void cplus::ast::ForeachStatement::accept(cplus::ast::ASTVisitor &visitor)
{
    visitor.visit(*this);
}

void cplus::ast::CaseStatement::accept(cplus::ast::ASTVisitor &visitor)
{
    visitor.visit(*this);
}

void cplus::ast::FunctionDeclaration::accept(cplus::ast::ASTVisitor &visitor)
{
    visitor.visit(*this);
}

void cplus::ast::Program::accept(cplus::ast::ASTVisitor &visitor)
{
    visitor.visit(*this);
}
