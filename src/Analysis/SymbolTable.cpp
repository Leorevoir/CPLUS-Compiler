#include <CPlus/Analysis/SymbolTable.hpp>
#include <CPlus/Arguments.hpp>
#include <CPlus/Logger.hpp>

#include <variant>

/**
* public
*/

std::unique_ptr<cplus::ast::Module> cplus::st::SymbolTable::run(const std::unique_ptr<ast::Module> &module)
{
    _module = module->name.c_str();
    if (cplus_flags & FLAG_DEBUG) {
        logger::info("SymbolTable::run ", "Building symbol table for module: ", _module);
    }

    _enter_scope();
    module->accept(*this);
    _exit_scope();

    if (!_scope_stack.empty()) {
        throw exception::Error("SymbolTable::run", "Scope stack not empty after processing module: ", _module);
    }

    return std::move(const_cast<std::unique_ptr<ast::Module> &>(module));
}

/**
 * helper
 */

static constexpr inline std::string_view _to_string(cplus::ast::Type::Kind k)
{
    switch (k) {
        case cplus::ast::Type::INT:
            return "int";
        case cplus::ast::Type::FLOAT:
            return "float";
        case cplus::ast::Type::STRING:
            return "string";
        case cplus::ast::Type::VOID:
            return "void";
        case cplus::ast::Type::AUTO:
            return "auto";
        default:
            return "unknown";
    }
}

static cplus::ast::TypePtr _make_type(const cplus::ast::Type::Kind k)
{
    return cplus::ast::make<cplus::ast::Type>(k, _to_string(k));
}

/**
 * private
 */

inline void cplus::st::SymbolTable::_enter_scope()
{
    auto new_scope = std::make_unique<st::Scope>(_current_scope);

    _current_scope = new_scope.get();
    _scope_stack.push_back(std::move(new_scope));
}

inline void cplus::st::SymbolTable::_exit_scope()
{
    if (!_scope_stack.empty()) {
        _scope_stack.pop_back();
        _current_scope = _scope_stack.empty() ? nullptr : _scope_stack.back().get();
    }
}

inline bool cplus::st::SymbolTable::_declare(const std::string &name, st::Symbol::Kind kind, ast::TypePtr type, bool is_const, u64 line,
    u64 column)
{
    if (!_current_scope) {
        return false;
    }
    auto symbol = std::make_unique<st::Symbol>(kind, name, std::move(type), is_const, line, column);

    return _current_scope->declare(name, std::move(symbol));
}

inline cplus::st::Symbol *cplus::st::SymbolTable::_lookup(const std::string &name)
{
    return _current_scope ? _current_scope->lookup(name) : nullptr;
}

/**
 * ast visit
 */

inline void cplus::st::SymbolTable::visit(ast::Module &node)
{
    for (const auto &decl : node.declarations) {
        decl->accept(*this);
    }
}

inline void cplus::st::SymbolTable::visit(ast::FunctionDeclaration &node)
{
    ast::TypePtr return_type =
        node.return_type ? ast::make<ast::Type>(node.return_type->kind, node.return_type->name) : ast::make<ast::Type>(ast::Type::VOID);

    if (!_declare(std::string(node.name), st::Symbol::FUNCTION, std::move(return_type), false, node.line, node.column)) {
        throw exception::Error("SymbolTable::visit", "Function \"", node.name, "\" already declared in module: ", _module, " at ",
            node.line, ":", node.column);
    }

    st::Symbol *func_sym = _lookup(std::string(node.name));

    if (!func_sym) {
        _return_type_stack.emplace_back(ast::make<ast::Type>(ast::Type::VOID));
    } else {
        for (const auto &param : node.parameters) {
            ast::TypePtr param_type =
                param.type ? ast::make<ast::Type>(param.type->kind, param.type->name) : ast::make<ast::Type>(ast::Type::AUTO);
            func_sym->param_types.push_back(std::move(param_type));
        }
        _return_type_stack.emplace_back(ast::make<ast::Type>(func_sym->type->kind, func_sym->type->name));
    }

    _enter_scope();

    for (const auto &param : node.parameters) {
        ast::TypePtr param_type =
            param.type ? ast::make<ast::Type>(param.type->kind, param.type->name) : ast::make<ast::Type>(ast::Type::AUTO);

        if (!_declare(std::string(param.name), st::Symbol::PARAMETER, std::move(param_type), false)) {
            throw exception::Error("SymbolTable::visit", "Parameter '", param.name, "' already declared in function '", node.name,
                " in module: ", _module, "' at ", node.line, ":", node.column);
        }
    }

    _has_return_stack.push_back(false);
    if (node.body) {
        node.body->accept(*this);
    }
    if (_return_type_stack.back()->kind != ast::Type::VOID && !_has_return_stack.back()) {
        throw exception::Error("SymbolTable::visit", "Non-void function '", node.name, "' must return a value in module: ", _module, " at ",
            node.line, ":", node.column);
    }

    _exit_scope();
    _has_return_stack.pop_back();
    _return_type_stack.pop_back();
}

inline void cplus::st::SymbolTable::visit(ast::VariableDeclaration &node)
{
    ast::TypePtr var_type;

    if (node.declared_type) {
        var_type = ast::make<ast::Type>(node.declared_type->kind, node.declared_type->name);
    } else if (node.initializer) {
        node.initializer->accept(*this);
        var_type = _infer_type(*node.initializer);
    } else {
        throw exception::Error("SymbolTable::visit", "Variable '", node.name, "' must have type or initializer in module: ", _module,
            " at ", node.line, ":", node.column);
    }

    if (!_declare(std::string(node.name), st::Symbol::VARIABLE, std::move(var_type), node.is_const, node.line, node.column)) {
        throw exception::Error("SymbolTable::visit", "Variable '", node.name, "' already declared in module: ", _module, " at ", node.line,
            ":", node.column);
    }

    if (node.initializer && node.declared_type) {
        node.initializer->accept(*this);

        const ast::Type *decl = node.declared_type.get();
        const ast::Type *init = node.initializer->type.get();

        if (!_is_compatible(decl, init)) {
            throw exception::Error("SymbolTable::visit", "Type mismatch in initializer for variable '", node.name, "' in module: ", _module,
                " at ", node.line, ":", node.column);
        }
    }
}

inline void cplus::st::SymbolTable::visit(ast::IdentifierExpression &node)
{
    const st::Symbol *symbol = _lookup(std::string(node.name));

    if (!symbol) {
        throw exception::Error("SymbolTable::visit", "Undefined identifier '", node.name, "' in module: ", _module, " at ", node.line, ":",
            node.column);
    }

    node.type = ast::make<ast::Type>(symbol->type->kind, symbol->type->name);
}

inline void cplus::st::SymbolTable::visit(ast::BlockStatement &node)
{
    _enter_scope();
    for (const auto &stmt : node.statements) {
        stmt->accept(*this);
    }
    _exit_scope();
}

inline void cplus::st::SymbolTable::visit(ast::LiteralExpression &node)
{
    if (std::holds_alternative<i32>(node.value)) {
        node.type = _make_type(ast::Type::INT);
    } else if (std::holds_alternative<f32>(node.value)) {
        node.type = _make_type(ast::Type::FLOAT);
    } else if (std::holds_alternative<std::string_view>(node.value)) {
        node.type = _make_type(ast::Type::STRING);
    } else {
        node.type = _make_type(ast::Type::AUTO);
    }
}

inline void cplus::st::SymbolTable::visit(ast::BinaryExpression &node)
{
    node.left->accept(*this);
    node.right->accept(*this);

    const ast::Type *left_type = node.left->type.get();
    const ast::Type *right_type = node.right->type.get();

    if (!_is_compatible(left_type, right_type)) {
        throw exception::Error("SymbolTable::visit", "Type mismatch in binary expression in module: ", _module, " at ", node.line, ":",
            node.column);
    }

    node.type = ast::make<ast::Type>(left_type->kind, left_type->name);
}

inline void cplus::st::SymbolTable::visit(ast::UnaryExpression &node)
{
    node.operand->accept(*this);
}

inline void cplus::st::SymbolTable::visit(ast::CallExpression &node)
{
    for (const auto &arg : node.arguments) {
        arg->accept(*this);
    }

    const st::Symbol *sym = _lookup(std::string(node.function_name));

    if (!sym || sym->kind != st::Symbol::FUNCTION) {
        throw exception::Error("SymbolTable::visit", "Call to undefined function '", node.function_name, "' in module: ", _module, " at ",
            node.line, ":", node.column);
    }

    if (!sym->param_types.empty() && sym->param_types.size() != node.arguments.size()) {
        throw exception::Error("SymbolTable::visit", "Wrong number of arguments when calling '", node.function_name,
            "' in module: ", _module, " at ", node.line, ":", node.column);
    }

    for (size_t i = 0; i < node.arguments.size() && i < sym->param_types.size(); ++i) {
        const ast::Type *expected = sym->param_types[i].get();
        const ast::Type *actual = node.arguments[i]->type.get();
        if (!expected || !actual) {
            throw exception::Error("SymbolTable::visit", "Unable to determine argument type for call to '", node.function_name, "' at ",
                node.line, ":", node.column);
        }
        if (!_is_compatible(expected, actual)) {
            throw exception::Error("SymbolTable::visit", "Argument type mismatch in call to '", node.function_name, "': expected ",
                expected->name.empty() ? "void" : expected->name, " got ", actual->name.empty() ? "void" : actual->name,
                " in module: ", _module, " at ", node.line, ":", node.column);
        }
    }

    if (sym->type) {
        node.type = ast::make<ast::Type>(sym->type->kind, sym->type->name);
    } else {
        node.type = ast::make<ast::Type>(ast::Type::AUTO);
    }
}

inline void cplus::st::SymbolTable::visit(ast::AssignmentExpression &node)
{
    node.value->accept(*this);
    const st::Symbol *symbol = _lookup(std::string(node.variable_name));

    if (!symbol) {
        throw exception::Error("SymbolTable::visit", "Assign to undefined variable '", node.variable_name, "' in module: ", _module, " at ",
            node.line, ":", node.column);
    }

    const ast::Type *dest = symbol->type.get();
    const ast::Type *src = node.value->type.get();

    if (!_is_compatible(dest, src)) {
        throw exception::Error("SymbolTable::visit", "Type mismatch in assignment to variable '", node.variable_name,
            "' in module: ", _module, " at ", node.line, ":", node.column);
    }

    node.type = ast::make<ast::Type>(dest->kind, dest->name);
}

inline void cplus::st::SymbolTable::visit(ast::ExpressionStatement &node)
{
    node.expression->accept(*this);
}

inline void cplus::st::SymbolTable::visit(ast::ReturnStatement &node)
{
    if (_return_type_stack.empty()) {
        throw exception::Error("SymbolTable::visit", "Return statement outside of function in module: ", _module, " at ", node.line, ":",
            node.column);
    }
    _has_return_stack.back() = true;

    if (node.value) {
        node.value->accept(*this);
        const ast::Type *expected = _return_type_stack.back().get();
        const ast::Type *actual = node.value->type.get();

        if (!expected || !actual) {
            throw exception::Error("SymbolTable::visit", "Unable to determine return type in module: ", _module, " at ", node.line, ":",
                node.column);
        }

        if (!_is_compatible(expected, actual)) {
            throw exception::Error("SymbolTable::visit", "Return type mismatch: expected ",
                expected->name.empty() ? "void" : expected->name, " got ", actual->name.empty() ? "void" : actual->name,
                " in module: ", _module, " at ", node.line, ":", node.column);
        }
    } else {
        const ast::Type *expected = _return_type_stack.back().get();
        if (expected && expected->kind != ast::Type::VOID) {
            throw exception::Error("SymbolTable::visit", "Return type mismatch: expected ",
                expected->name.empty() ? "void" : expected->name, " got void in module: ", _module, " at ", node.line, ":", node.column);
        }
    }
}

inline void cplus::st::SymbolTable::visit(ast::IfStatement &node)
{
    node.condition->accept(*this);
    node.then_statement->accept(*this);
    if (node.else_statement) {
        node.else_statement->accept(*this);
    }
}

inline void cplus::st::SymbolTable::visit(ast::ForStatement &node)
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

inline void cplus::st::SymbolTable::visit(ast::ForeachStatement &node)
{
    _enter_scope();

    if (node.iterable) {
        node.iterable->accept(*this);
    }

    //TODO infer the type of the iterator variable based on the iterable's type
    ast::TypePtr iter_type = _make_type(ast::Type::AUTO);
    const std::string iterator_name = std::string(node.iterator_name);

    if (!_declare(iterator_name, st::Symbol::VARIABLE, std::move(iter_type), false, node.line, node.column)) {
        throw exception::Error("SymbolTable::visit", "Variable '", iterator_name, "' already declared in foreach in module: ", _module,
            " at ", node.line, ":", node.column);
    }

    if (node.body) {
        node.body->accept(*this);
    }

    _exit_scope();
}

inline void cplus::st::SymbolTable::visit(ast::CaseStatement &node)
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

cplus::ast::TypePtr cplus::st::SymbolTable::_infer_type(ast::Expression &expr)
{
    if (expr.type) {
        return ast::make<ast::Type>(expr.type->kind, expr.type->name);
    }

    //TODO: find a better way
    if (const auto lit = dynamic_cast<ast::LiteralExpression *>(&expr)) {
        if (std::holds_alternative<i32>(lit->value)) {
            return _make_type(ast::Type::INT);
        } else if (std::holds_alternative<f32>(lit->value)) {
            return _make_type(ast::Type::FLOAT);
        } else if (std::holds_alternative<std::string_view>(lit->value)) {
            return _make_type(ast::Type::STRING);
        }
    }

    if (const auto bin = dynamic_cast<ast::BinaryExpression *>(&expr)) {
        if (bin->left && bin->left->type) {
            return ast::make<ast::Type>(bin->left->type->kind, bin->left->type->name);
        }
    }

    if (const auto call = dynamic_cast<ast::CallExpression *>(&expr)) {
        const st::Symbol *sym = _lookup(std::string(call->function_name));
        if (sym && sym->kind == st::Symbol::FUNCTION && sym->type) {
            return ast::make<ast::Type>(sym->type->kind, sym->type->name);
        }
    }

    if (const auto ident = dynamic_cast<ast::IdentifierExpression *>(&expr)) {
        const st::Symbol *sym = _lookup(std::string(ident->name));
        if (sym && sym->type) {
            return ast::make<ast::Type>(sym->type->kind, sym->type->name);
        }
    }

    return ast::make<ast::Type>(ast::Type::AUTO);
}

inline bool cplus::st::SymbolTable::_is_compatible(const ast::Type *left, const ast::Type *right)
{
    if (!left || !right) {
        return false;
    }
    return left->kind == right->kind;
}
