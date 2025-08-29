#include <CPlus/Logger.hpp>
#include <CPlus/Parser/Logger.hpp>

/**
 * public
 */

cplus::ast::ASTLogger::ASTLogger(std::ostream &out) : _out(out)
{
    /* __ctor__ */
}

/**
 * helpers (stringify)
 */

static constexpr std::string _to_string(const cplus::ast::BinaryExpression::Operator op)
{
    {
        switch (op) {
            case cplus::ast::BinaryExpression::ADD:
                return "+";
            case cplus::ast::BinaryExpression::SUB:
                return "-";
            case cplus::ast::BinaryExpression::MUL:
                return "*";
            case cplus::ast::BinaryExpression::DIV:
                return "/";
            case cplus::ast::BinaryExpression::MOD:
                return "%";
            case cplus::ast::BinaryExpression::EQ:
                return "==";
            case cplus::ast::BinaryExpression::NEQ:
                return "!=";
            case cplus::ast::BinaryExpression::LT:
                return "<";
            case cplus::ast::BinaryExpression::LTE:
                return "<=";
            case cplus::ast::BinaryExpression::GT:
                return ">";
            case cplus::ast::BinaryExpression::GTE:
                return ">=";
            case cplus::ast::BinaryExpression::AND:
                return "&&";
            case cplus::ast::BinaryExpression::OR:
                return "||";
            default:
                return "?";
        }
    }
}

static constexpr std::string _to_string(const cplus::ast::UnaryExpression::Operator op)
{
    switch (op) {
        case cplus::ast::UnaryExpression::NOT:
            return "!";
        case cplus::ast::UnaryExpression::NEGATE:
            return "-";
        case cplus::ast::UnaryExpression::PLUS:
            return "+";
        case cplus::ast::UnaryExpression::INC:
            return "++";
        case cplus::ast::UnaryExpression::DEC:
            return "--";
        default:
            return "?";
    }
}

/**
 * public
 */

void cplus::ast::ASTLogger::show(ASTNode &node)
{
    _indent = 0;
    node.accept(*this);
}

void cplus::ast::ASTLogger::visit(LiteralExpression &node)
{
    _out << logger::CPLUS_GREEN;
    _show_indent("Literal");
    _out << logger::CPLUS_RESET;
    std::visit([&](auto &&val) { _out << logger::CPLUS_CYAN << " = " << logger::CPLUS_RESET << val << "\n"; }, node.value);
}

void cplus::ast::ASTLogger::visit(IdentifierExpression &node)
{
    _out << logger::CPLUS_YELLOW;
    _show_indent("Identifier: " + std::string(node.name) + "\n");
    _out << logger::CPLUS_RESET;
}

void cplus::ast::ASTLogger::visit(BinaryExpression &node)
{
    _show_indent("BinaryExpression\n");
    _push();
    node.left->accept(*this);
    _out << logger::CPLUS_CYAN;
    _show_indent(_to_string(node.op) + "\n");
    _out << logger::CPLUS_RESET;
    node.right->accept(*this);
    _pop();
}

void cplus::ast::ASTLogger::visit(UnaryExpression &node)
{
    _out << logger::CPLUS_BLUE;
    _show_indent("UnaryExpression " + std::string(logger::CPLUS_CYAN) + _to_string(node.op) + "\n");
    _out << logger::CPLUS_RESET;
    _push();
    node.operand->accept(*this);
    _pop();
}

void cplus::ast::ASTLogger::visit(CallExpression &node)
{
    _out << logger::CPLUS_BLUE;
    _show_indent("Call: " + std::string(node.function_name) + "\n");
    _out << logger::CPLUS_RESET;
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
    _out << logger::CPLUS_CYAN;
    _show_indent("VarDecl: " + std::string(node.name) + "\n");
    _out << logger::CPLUS_RESET;
    _push();
    if (node.initializer)
        node.initializer->accept(*this);
    _pop();
}

void cplus::ast::ASTLogger::visit(ReturnStatement &node)
{
    _out << logger::CPLUS_MAGENTA;
    _show_indent("Return\n");
    _out << logger::CPLUS_RESET;
    if (node.value) {
        _push();
        node.value->accept(*this);
        _pop();
    }
}

void cplus::ast::ASTLogger::visit(IfStatement &node)
{
    _out << logger::CPLUS_MAGENTA;
    _show_indent(+"If\n");
    _out << logger::CPLUS_RESET;
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
        _out << logger::CPLUS_MAGENTA;
        _show_indent("Else:\n");
        _out << logger::CPLUS_RESET;
        _push();
        node.else_statement->accept(*this);
        _pop();
    }
    _pop();
}

void cplus::ast::ASTLogger::visit(ForStatement &node)
{
    _out << logger::CPLUS_MAGENTA;
    _show_indent("For\n");
    _out << logger::CPLUS_RESET;
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
    _out << logger::CPLUS_MAGENTA;
    _show_indent("Foreach " + std::string(node.iterator_name) + "\n");
    _out << logger::CPLUS_RESET;
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

void cplus::ast::ASTLogger::visit(CaseStatement &node)
{
    _out << logger::CPLUS_MAGENTA;
    _show_indent("Case\n");
    _out << logger::CPLUS_RESET;
    _push();

    if (node.expression) {
        _show_indent("Expression:\n");
        _push();
        node.expression->accept(*this);
        _pop();
    }

    for (const auto &clause : node.clauses) {
        if (clause.value) {
            _out << logger::CPLUS_MAGENTA;
            _show_indent("Case:\n");
            _out << logger::CPLUS_RESET;
            _push();
            clause.value->accept(*this);
            _pop();
        } else {
            _out << logger::CPLUS_MAGENTA;
            _show_indent("Default:\n");
            _out << logger::CPLUS_RESET;
        }

        _show_indent("Statements:\n");
        _push();
        for (const auto &stmt : clause.statements) {
            stmt->accept(*this);
        }
        _pop();
    }

    _pop();
}

void cplus::ast::ASTLogger::visit(FunctionDeclaration &node)
{
    _show_indent(std::string(logger::CPLUS_RED) + "Function " + std::string(logger::CPLUS_BLUE) + std::string(node.name) + "\n");
    _out << logger::CPLUS_RESET;
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
    _out << logger::CPLUS_RED_BOLD;
    _show_indent("Program\n");
    _out << logger::CPLUS_RESET;
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
