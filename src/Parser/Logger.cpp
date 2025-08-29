#include <CPlus/Parser/Logger.hpp>

/**
 * public
 */

cplus::ast::ASTLogger::ASTLogger(std::ostream &out) : _out(out)
{
    /* __ctor__ */
}

void cplus::ast::ASTLogger::show(ASTNode &node)
{
    _indent = 0;
    node.accept(*this);
}

void cplus::ast::ASTLogger::visit(LiteralExpression &node)
{
    _show_indent("Literal");
    std::visit([&](auto &&val) { _out << " = " << val << "\n"; }, node.value);
}

void cplus::ast::ASTLogger::visit(IdentifierExpression &node)
{
    _show_indent("Identifier: " + std::string(node.name) + "\n");
}

void cplus::ast::ASTLogger::visit(BinaryExpression &node)
{
    _show_indent("BinaryExpression\n");
    _push();
    node.left->accept(*this);
    _show_indent(_to_string(node.op) + "\n");
    node.right->accept(*this);
    _pop();
}

void cplus::ast::ASTLogger::visit(UnaryExpression &node)
{
    _show_indent("UnaryExpression " + _to_string(node.op) + "\n");
    _push();
    node.operand->accept(*this);
    _pop();
}

void cplus::ast::ASTLogger::visit(CallExpression &node)
{
    _show_indent("Call: " + std::string(node.function_name) + "\n");
    _push();
    for (auto &arg : node.arguments)
        arg->accept(*this);
    _pop();
}

void cplus::ast::ASTLogger::visit(AssignmentExpression &node)
{
    _show_indent("Assign: " + std::string(node.variable_name) + "\n");
    _push();
    node.value->accept(*this);
    _pop();
}

/** Statements **/
void cplus::ast::ASTLogger::visit(ExpressionStatement &node)
{
    _show_indent("ExprStmt\n");
    _push();
    node.expression->accept(*this);
    _pop();
}

void cplus::ast::ASTLogger::visit(BlockStatement &node)
{
    _show_indent("Block\n");
    _push();
    for (auto &stmt : node.statements)
        stmt->accept(*this);
    _pop();
}

void cplus::ast::ASTLogger::visit(VariableDeclaration &node)
{
    _show_indent("VarDecl: " + std::string(node.name) + "\n");
    _push();
    if (node.initializer)
        node.initializer->accept(*this);
    _pop();
}

void cplus::ast::ASTLogger::visit(ReturnStatement &node)
{
    _show_indent("Return\n");
    if (node.value) {
        _push();
        node.value->accept(*this);
        _pop();
    }
}

void cplus::ast::ASTLogger::visit(IfStatement &node)
{
    _show_indent("If\n");
    _push();
    _show_indent("Condition:\n");
    _push();
    node.condition->accept(*this);
    _pop();
    _show_indent("Then:\n");
    _push();
    node.then_statement->accept(*this);
    _pop();
    if (node.else_statement) {
        _show_indent("Else:\n");
        _push();
        node.else_statement->accept(*this);
        _pop();
    }
    _pop();
}

void cplus::ast::ASTLogger::visit(ForStatement &node)
{
    _show_indent("For\n");
    _push();

    if (node.initializer) {
        _show_indent("Initializer:\n");
        _push();
        node.initializer->accept(*this);
        _pop();
    }

    if (node.condition) {
        _show_indent("Condition:\n");
        _push();
        node.condition->accept(*this);
        _pop();
    }

    if (node.increment) {
        _show_indent("Increment:\n");
        _push();
        node.increment->accept(*this);
        _pop();
    }

    if (node.body) {
        _show_indent("Body:\n");
        _push();
        node.body->accept(*this);
        _pop();
    }

    _pop();
}

void cplus::ast::ASTLogger::visit(ForeachStatement &node)
{
    _show_indent("Foreach " + std::string(node.iterator_name) + "\n");
    _push();

    if (node.iterable) {
        _show_indent("Iterable:\n");
        _push();
        node.iterable->accept(*this);
        _pop();
    }

    if (node.body) {
        _show_indent("Body:\n");
        _push();
        node.body->accept(*this);
        _pop();
    }

    _pop();
}

void cplus::ast::ASTLogger::visit(CaseStatement __attribute__((unused)) & node)
{
    _show_indent("Case\n");
}

void cplus::ast::ASTLogger::visit(FunctionDeclaration &node)
{
    _show_indent("Function " + std::string(node.name) + "\n");
    _push();
    for (auto &p : node.parameters) {
        _show_indent("Param: " + std::string(p.name) + "\n");
    }
    if (node.body) {
        node.body->accept(*this);
    }
    _pop();
}

void cplus::ast::ASTLogger::visit(Program &node)
{
    _show_indent("Program\n");
    _push();
    for (auto &decl : node.declarations)
        decl->accept(*this);
    _pop();
}

/**
* private
*/

void cplus::ast::ASTLogger::_show_indent(const std::string &text)
{
    _out << std::string(static_cast<u32>(_indent * 2), ' ') << text;
}

void cplus::ast::ASTLogger::_push()
{
    ++_indent;
}
void cplus::ast::ASTLogger::_pop()
{
    --_indent;
}

std::string cplus::ast::ASTLogger::_to_string(const BinaryExpression::Operator op)
{
    {
        switch (op) {
            case BinaryExpression::ADD:
                return "+";
            case BinaryExpression::SUB:
                return "-";
            case BinaryExpression::MUL:
                return "*";
            case BinaryExpression::DIV:
                return "/";
            case BinaryExpression::MOD:
                return "%";
            case BinaryExpression::EQ:
                return "==";
            case BinaryExpression::NEQ:
                return "!=";
            case BinaryExpression::LT:
                return "<";
            case BinaryExpression::LTE:
                return "<=";
            case BinaryExpression::GT:
                return ">";
            case BinaryExpression::GTE:
                return ">=";
            case BinaryExpression::AND:
                return "&&";
            case BinaryExpression::OR:
                return "||";
            default:
                return "?";
        }
    }
}

std::string cplus::ast::ASTLogger::_to_string(const UnaryExpression::Operator op)
{
    switch (op) {
        case UnaryExpression::NOT:
            return "!";
        case UnaryExpression::NEGATE:
            return "-";
        case UnaryExpression::PLUS:
            return "+";
        default:
            return "?";
    }
}
