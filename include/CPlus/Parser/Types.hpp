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
struct Type;

using ASTNodePtr = std::unique_ptr<ASTNode>;
using ExpressionPtr = std::unique_ptr<Expression>;
using StatementPtr = std::unique_ptr<Statement>;
using TypePtr = std::unique_ptr<Type>;

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
        std::string_view name;

        inline Type(const Kind k) : kind(k)
        {
            /* __ctor__ */
        }

        inline Type(Kind k, const std::string_view &n) : kind(k), name(n)
        {
            /* __ctor__ */
        }
};

static inline constexpr Type::Kind from_string(const std::string_view &str)
{
    if (str == "int") {
        return Type::INT;
    } else if (str == "float") {
        return Type::FLOAT;
    } else if (str == "string") {
        return Type::STRING;
    } else if (str == "bool") {
        return Type::BOOL;
    } else if (str == "void") {
        return Type::VOID;
    } else {
        return Type::AUTO;
    }
}

static inline constexpr cstr to_string(const Type::Kind kind)
{
    switch (kind) {
        case Type::INT:
            return "int";
        case Type::FLOAT:
            return "float";
        case Type::STRING:
            return "string";
        case Type::BOOL:
            return "bool";
        case Type::VOID:
            return "void";
        case Type::AUTO:
            return "auto";
        default:
            return "unknown";
    }
}

class Expression : public ASTNode
{
    public:
        TypePtr type;
};

class Statement : public ASTNode
{
        /* __statement__ */
};

class LiteralExpression : public Expression
{
    public:
        std::variant<i64, double, std::string_view, bool> value;

        inline LiteralExpression(i64 val) : value(val)
        {
            /* __ctor__ */
        }

        inline LiteralExpression(double val) : value(val)
        {
            /* __ctor__ */
        }

        inline LiteralExpression(const std::string_view &val) : value(val)
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
        std::string_view name;

        inline IdentifierExpression(const std::string_view &n) : name(n)
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
        enum Operator { NOT, NEGATE, PLUS, INC, DEC };

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
        std::string_view function_name;
        std::vector<ExpressionPtr> arguments;

        inline CallExpression(const std::string_view &name) : function_name(name)
        {
            /* __ctor__ */
        }

        void accept(ASTVisitor &visitor) override;
};

class AssignmentExpression : public Expression
{
    public:
        std::string_view variable_name;
        ExpressionPtr value;

        inline AssignmentExpression(const std::string_view &name, ExpressionPtr val) : variable_name(name), value(std::move(val))
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
        std::string_view name;
        TypePtr declared_type;    //<< nullptr for auto-deduced
        ExpressionPtr initializer;//<< nullptr if no initializer
        bool is_const = false;

        inline VariableDeclaration(const std::string_view &n, bool is_const_ = false) : name(n), is_const(is_const_)
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
        std::string_view iterator_name;
        ExpressionPtr iterable;
        StatementPtr body;

        inline ForeachStatement(const std::string_view &iter, ExpressionPtr it, StatementPtr b)
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
        std::string_view name;
        TypePtr type;

        inline Parameter(const std::string_view &n, TypePtr t) : name(n), type(std::move(t))
        {
            /* __ctor__ */
        }
};

class FunctionDeclaration : public Statement
{
    public:
        std::string_view name;
        std::vector<Parameter> parameters;
        TypePtr return_type;
        StatementPtr body;

        inline FunctionDeclaration(const std::string_view &n) : name(n)
        {
            /* __ctor__ */
        }
        void accept(ASTVisitor &visitor) override;
};

class Module : public ASTNode
{
    public:
        std::string name;
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
        virtual void visit(Module &node) = 0;
};

template<typename T, typename... Args>
static inline std::unique_ptr<T> make(Args &&...args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

}// namespace ast

}// namespace cplus
