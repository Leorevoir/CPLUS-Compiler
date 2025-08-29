#include <CPlus/Analysis/SymbolTable.hpp>
#include <CPlus/Arguments.hpp>
#include <CPlus/Logger.hpp>

/**
* public
*/

inline std::unique_ptr<cplus::ast::Program> cplus::SymbolTable::run(const std::unique_ptr<ast::Program> &program)
{
    if (cplus_flags & FLAG_DEBUG) {
        logger::info("SymbolTable::run ", "Building symbol table...");
    }

    _enter_scope();
    program->accept(*this);
    _exit_scope();

    return std::move(const_cast<std::unique_ptr<ast::Program> &>(program));
}

/**
 * private
 */

inline void cplus::SymbolTable::_enter_scope()
{
    auto new_scope = std::make_unique<st::Scope>(current_scope);

    current_scope = new_scope.get();
    scope_stack.push_back(std::move(new_scope));
}

inline void cplus::SymbolTable::_exit_scope()
{
    if (!scope_stack.empty()) {
        scope_stack.pop_back();
        current_scope = scope_stack.empty() ? nullptr : scope_stack.back().get();
    }
}

inline bool cplus::SymbolTable::_declare(const std::string &name, st::Symbol::Kind kind, ast::TypePtr type, bool is_const, u64 line,
    u64 column)
{
    if (!current_scope) {
        return false;
    }
    auto symbol = std::make_unique<st::Symbol>(kind, name, std::move(type), is_const, line, column);

    return current_scope->declare(name, std::move(symbol));
}

inline cplus::st::Symbol *cplus::SymbolTable::_lookup(const std::string &name)
{
    return current_scope ? current_scope->lookup(name) : nullptr;
}

/**
 * ast visit
 */

inline void cplus::SymbolTable::visit(ast::Program &node)
{
    for (const auto &decl : node.declarations) {
        decl->accept(*this);
    }
}

inline void cplus::SymbolTable::visit(ast::FunctionDeclaration &node)
{
    ast::TypePtr return_type =
        node.return_type ? ast::make<ast::Type>(node.return_type->kind, node.return_type->name) : ast::make<ast::Type>(ast::Type::VOID);

    if (!_declare(std::string(node.name), st::Symbol::FUNCTION, std::move(return_type), false, node.line, node.column)) {
        throw exception::Error("SymbolTable::visit", "Function \"", node.name, "\" already declared at ", node.line, ":", node.column);
    }

    _enter_scope();

    for (const auto &param : node.parameters) {
        ast::TypePtr param_type =
            param.type ? ast::make<ast::Type>(param.type->kind, param.type->name) : ast::make<ast::Type>(ast::Type::AUTO);

        if (!_declare(std::string(param.name), st::Symbol::PARAMETER, std::move(param_type), false)) {
            throw exception::Error("SymbolTable::visit", "Parameter '", param.name, "' already declared in function '", node.name, "' at ",
                node.line, ":", node.column);
        }
    }

    if (node.body) {
        node.body->accept(*this);
    }

    _exit_scope();
}

inline void cplus::SymbolTable::visit(ast::VariableDeclaration &node)
{
    ast::TypePtr var_type;

    if (node.declared_type) {
        var_type = ast::make<ast::Type>(node.declared_type->kind, node.declared_type->name);
    } else if (node.initializer) {
        var_type = _infer_type(*node.initializer);
    } else {
        throw exception::Error("SymbolTable::visit", "Variable '", node.name, "' must have type or initializer at ", node.line, ":",
            node.column);
    }

    if (!_declare(std::string(node.name), st::Symbol::VARIABLE, std::move(var_type), node.is_const, node.line, node.column)) {
        throw exception::Error("SymbolTable::visit", "Variable '", node.name, "' already declared at ", node.line, ":", node.column);
    }

    if (node.initializer) {
        node.initializer->accept(*this);
    }
}

inline void cplus::SymbolTable::visit(ast::IdentifierExpression &node)
{
    st::Symbol *symbol = _lookup(std::string(node.name));

    if (!symbol) {
        throw exception::Error("SymbolTable::visit", "Undefined identifier '", node.name, "' at ", node.line, ":", node.column);
    }

    node.type = ast::make<ast::Type>(symbol->type->kind, symbol->type->name);
}

inline void cplus::SymbolTable::visit(ast::BlockStatement &node)
{
    _enter_scope();
    for (const auto &stmt : node.statements) {
        stmt->accept(*this);
    }
    _exit_scope();
}

inline void cplus::SymbolTable::visit(ast::LiteralExpression __attribute__((unused)) & node)
{
    /* TODO: type inference */
}

inline void cplus::SymbolTable::visit(ast::BinaryExpression &node)
{
    node.left->accept(*this);
    node.right->accept(*this);
}

inline void cplus::SymbolTable::visit(ast::UnaryExpression &node)
{
    node.operand->accept(*this);
}

inline void cplus::SymbolTable::visit(ast::CallExpression &node)
{
    for (const auto &arg : node.arguments) {
        arg->accept(*this);
    }
}

inline void cplus::SymbolTable::visit(ast::AssignmentExpression &node)
{
    node.value->accept(*this);
}

inline void cplus::SymbolTable::visit(ast::ExpressionStatement &node)
{
    node.expression->accept(*this);
}

inline void cplus::SymbolTable::visit(ast::ReturnStatement &node)
{
    if (node.value) {
        node.value->accept(*this);
    }
}

inline void cplus::SymbolTable::visit(ast::IfStatement &node)
{
    node.condition->accept(*this);
    node.then_statement->accept(*this);
    if (node.else_statement) {
        node.else_statement->accept(*this);
    }
}

inline void cplus::SymbolTable::visit(ast::ForStatement &node)
{
    _enter_scope();
    if (node.initializer) {
        node.initializer->accept(*this);
    }
    if (node.condition) {
        node.condition->accept(*this);
    }
    if (node.increment) {
        node.increment->accept(*this);
    }
    if (node.body) {
        node.body->accept(*this);
    }
    _exit_scope();
}

inline void cplus::SymbolTable::visit(ast::ForeachStatement &node)
{
    _enter_scope();

    if (node.iterable) {
        node.iterable->accept(*this);
    }

    //TODO infer the type of the iterator variable based on the iterable's type
    ast::TypePtr iter_type = ast::make<ast::Type>(ast::Type::AUTO);
    const std::string iterator_name = std::string(node.iterator_name);

    if (!_declare(iterator_name, st::Symbol::VARIABLE, std::move(iter_type), false, node.line, node.column)) {
        throw exception::Error("SymbolTable::visit", "Variable '", iterator_name, "' already declared in foreach at ", node.line, ":",
            node.column);
    }

    if (node.body) {
        node.body->accept(*this);
    }

    _exit_scope();
}

inline void cplus::SymbolTable::visit(ast::CaseStatement &node)
{
    node.expression->accept(*this);
    for (const auto &clause : node.clauses) {
        if (clause.value) {
            clause.value->accept(*this);
        }
        for (const auto &stmt : clause.statements) {
            stmt->accept(*this);
        }
    }
}

cplus::ast::TypePtr cplus::SymbolTable::_infer_type(ast::Expression __attribute__((unused)) & expr)
{
    //TODO
    return ast::make<ast::Type>(ast::Type::AUTO);
}

inline bool cplus::SymbolTable::_is_compatible(const ast::Type *left, const ast::Type *right)
{
    if (!left || !right) {
        return false;
    }
    return left->kind == right->kind;
}
