#pragma once

#include <CPlus/Types.hpp>

#include <memory>
#include <variant>
#include <vector>

namespace cplus {

namespace ast {

class ASTNode;
class Expression;
class Statement;

using ASTNodePtr = std::unique_ptr<ASTNode>;
using ExpressionPtr = std::unique_ptr<Expression>;
using StatementPtr = std::unique_ptr<Statement>;

class ASTNode
{
    public:
        virtual ~ASTNode() = default;
        virtual void accept(class ASTVisitor &visitor) = 0;

        u64 line = 0;
        u64 column = 0;
};

struct Type {
        enum Kind { INT, FLOAT, STRING, BOOL, VOID, AUTO };

        Kind kind;
        std::string name;

        inline Type(const Kind k) : kind(k)
        {
            /* __ctor__ */
        }

        inline Type(Kind k, const std::string &n) : kind(k), name(n)
        {
            /* __ctor__ */
        }
};

class Expression : public ASTNode
{
    public:
        std::unique_ptr<Type> type;
};

class Statement : public ASTNode
{
        /* __statement__ */
};

class LiteralExpression : public Expression
{
    public:
        std::variant<i64, double, std::string, bool> value;

        inline LiteralExpression(i64 val) : value(val)
        {
            /* __ctor__ */
        }

        inline LiteralExpression(double val) : value(val)
        {
            /* __ctor__ */
        }

        inline LiteralExpression(const std::string &val) : value(val)
        {
            /* __ctor__ */
        }

        inline LiteralExpression(bool val) : value(val)
        {
            /* __ctor__ */
        }

        void accept(ASTVisitor &visitor) override;
};

class IdentifierExpression : public Expression
{
    public:
        std::string name;

        inline IdentifierExpression(const std::string &n) : name(n)
        {
            /* __ctor__ */
        }

        void accept(ASTVisitor &visitor) override;
};

class BinaryExpression : public Expression
{
    public:
        enum Operator { ADD, SUB, MUL, DIV, MOD, EQ, NEQ, LT, LTE, GT, GTE, AND, OR };

        ExpressionPtr left;
        Operator op;
        ExpressionPtr right;

        inline BinaryExpression(ExpressionPtr l, Operator o, ExpressionPtr r) : left(std::move(l)), op(o), right(std::move(r))
        {
            /* __ctor__ */
        }

        void accept(ASTVisitor &visitor) override;
};

class UnaryExpression : public Expression
{
    public:
        enum Operator { NOT, NEGATE, PLUS };

        Operator op;
        ExpressionPtr operand;

        inline UnaryExpression(Operator o, ExpressionPtr expr) : op(o), operand(std::move(expr))
        {
            /* __ctor__ */
        }

        void accept(ASTVisitor &visitor) override;
};

class CallExpression : public Expression
{
    public:
        std::string function_name;
        std::vector<ExpressionPtr> arguments;

        inline CallExpression(const std::string &name) : function_name(name)
        {
            /* __ctor__ */
        }

        void accept(ASTVisitor &visitor) override;
};

class AssignmentExpression : public Expression
{
    public:
        std::string variable_name;
        ExpressionPtr value;

        inline AssignmentExpression(const std::string &name, ExpressionPtr val) : variable_name(name), value(std::move(val))
        {
            /* __ctor__ */
        }

        void accept(ASTVisitor &visitor) override;
};

class ExpressionStatement : public Statement
{
    public:
        ExpressionPtr expression;

        inline ExpressionStatement(ExpressionPtr expr) : expression(std::move(expr))
        {
            /* __ctor__ */
        }

        void accept(ASTVisitor &visitor) override;
};

class BlockStatement : public Statement
{
    public:
        std::vector<StatementPtr> statements;

        void accept(ASTVisitor &visitor) override;
};

class VariableDeclaration : public Statement
{
    public:
        std::string name;
        std::unique_ptr<Type> declared_type;//<< nullptr for auto-deduced
        ExpressionPtr initializer;          //<< nullptr if no initializer

        inline VariableDeclaration(const std::string &n) : name(n)
        {
            /* __ctor__ */
        }

        void accept(ASTVisitor &visitor) override;
};

class ReturnStatement : public Statement
{
    public:
        ExpressionPtr value;//<< nullptr for void return

        inline ReturnStatement(ExpressionPtr val = nullptr) : value(std::move(val))
        {
            /* __ctor__ */
        }

        void accept(ASTVisitor &visitor) override;
};

class IfStatement : public Statement
{
    public:
        ExpressionPtr condition;
        StatementPtr then_statement;
        StatementPtr else_statement;//<< nullptr if no else

        inline IfStatement(ExpressionPtr cond, StatementPtr then_stmt) : condition(std::move(cond)), then_statement(std::move(then_stmt))
        {
            /* __ctor__ */
        }

        void accept(ASTVisitor &visitor) override;
};

class ForStatement : public Statement
{
    public:
        StatementPtr initializer;
        ExpressionPtr condition;
        ExpressionPtr increment;
        StatementPtr body;

        void accept(ASTVisitor &visitor) override;
};

class ForeachStatement : public Statement
{
    public:
        std::string iterator_name;
        ExpressionPtr iterable;
        StatementPtr body;

        inline ForeachStatement(const std::string &iter, ExpressionPtr it, StatementPtr b)
            : iterator_name(iter), iterable(std::move(it)), body(std::move(b))
        {
            /* __ctor__ */
        }

        void accept(ASTVisitor &visitor) override;
};

class CaseStatement : public Statement
{
    public:
        struct CaseClause {
                ExpressionPtr value;//<< default case is nullptr
                std::vector<StatementPtr> statements;
        };

        ExpressionPtr expression;
        std::vector<CaseClause> clauses;

        inline CaseStatement(ExpressionPtr expr) : expression(std::move(expr))
        {
            /* __ctor__ */
        }

        void accept(ASTVisitor &visitor) override;
};

class Parameter
{
    public:
        std::string name;
        std::unique_ptr<Type> type;

        inline Parameter(const std::string &n, std::unique_ptr<Type> t) : name(n), type(std::move(t))
        {
            /* __ctor__ */
        }
};

class FunctionDeclaration : public Statement
{
    public:
        std::string name;
        std::vector<Parameter> parameters;
        std::unique_ptr<Type> return_type;
        StatementPtr body;

        inline FunctionDeclaration(const std::string &n) : name(n)
        {
            /* __ctor__ */
        }
        void accept(ASTVisitor &visitor) override;
};

class Program : public ASTNode
{
    public:
        std::vector<StatementPtr> declarations;

        void accept(ASTVisitor &visitor) override;
};

class ASTVisitor
{
    public:
        virtual ~ASTVisitor() = default;

        /** @brief expressions */
        virtual void visit(LiteralExpression &node) = 0;
        virtual void visit(IdentifierExpression &node) = 0;
        virtual void visit(BinaryExpression &node) = 0;
        virtual void visit(UnaryExpression &node) = 0;
        virtual void visit(CallExpression &node) = 0;
        virtual void visit(AssignmentExpression &node) = 0;

        /** @brief statements */
        virtual void visit(ExpressionStatement &node) = 0;
        virtual void visit(BlockStatement &node) = 0;
        virtual void visit(VariableDeclaration &node) = 0;
        virtual void visit(ReturnStatement &node) = 0;
        virtual void visit(IfStatement &node) = 0;
        virtual void visit(ForStatement &node) = 0;
        virtual void visit(ForeachStatement &node) = 0;
        virtual void visit(CaseStatement &node) = 0;
        virtual void visit(FunctionDeclaration &node) = 0;
        virtual void visit(Program &node) = 0;
};

template<typename T, typename... Args>
static inline std::unique_ptr<T> make(Args &&...args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

}// namespace ast

}// namespace cplus
