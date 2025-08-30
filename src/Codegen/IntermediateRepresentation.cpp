#include <CPlus/Codegen/IntermediateRepresentation.hpp>
#include <CPlus/Logger.hpp>

/**
 * public
 */

const std::string cplus::ir::IntermediateRepresentation::run(const std::unique_ptr<ast::Module> &module)
{
    _temp_counter = 0;
    _label_counter = 0;
    _output.clear();
    _last_value.clear();
    _current_function.clear();

    emit("; C+ generated IR for module " + module->name);
    module->accept(*this);

    return _output;
}

/**
 * helpers
 */

static inline constexpr cplus::cstr binary_op_to_string(const cplus::ast::BinaryExpression::Operator op)
{
    switch (op) {
        case cplus::ast::BinaryExpression::ADD:
            return "add";
        case cplus::ast::BinaryExpression::SUB:
            return "sub";
        case cplus::ast::BinaryExpression::MUL:
            return "mul";
        case cplus::ast::BinaryExpression::DIV:
            return "sdiv";
        case cplus::ast::BinaryExpression::MOD:
            return "srem";
        case cplus::ast::BinaryExpression::EQ:
            return "icmp.eq";
        case cplus::ast::BinaryExpression::NEQ:
            return "icmp.ne";
        case cplus::ast::BinaryExpression::LT:
            return "icmp.slt";
        case cplus::ast::BinaryExpression::LTE:
            return "icmp.sle";
        case cplus::ast::BinaryExpression::GT:
            return "icmp.sgt";
        case cplus::ast::BinaryExpression::GTE:
            return "icmp.sge";
        case cplus::ast::BinaryExpression::AND:
            return "and";
        case cplus::ast::BinaryExpression::OR:
            return "or";
        default:
            return "op_unknown";
    }
}

static inline constexpr cplus::cstr unary_op_to_string(const cplus::ast::UnaryExpression::Operator op)
{
    switch (op) {
        case cplus::ast::UnaryExpression::NOT:
            return "not";
        case cplus::ast::UnaryExpression::NEGATE:
            return "neg";
        case cplus::ast::UnaryExpression::PLUS:
            return "plus";
        case cplus::ast::UnaryExpression::INC:
            return "inc";
        case cplus::ast::UnaryExpression::DEC:
            return "dec";
        default:
            return "unary_unknown";
    }
}

/**
* private
*/

void cplus::ir::IntermediateRepresentation::emit(const std::string &s)
{
    _output += s;
    _output += '\n';
}

std::string cplus::ir::IntermediateRepresentation::new_temp(const std::string &hint)
{
    return "%" + hint + std::to_string(_temp_counter++);
}

std::string cplus::ir::IntermediateRepresentation::new_label(const std::string &hint)
{
    return hint + std::to_string(_label_counter++);
}

/**
* AST visitor
*/

void cplus::ir::IntermediateRepresentation::visit(ast::LiteralExpression &node)
{
    if (std::holds_alternative<i32>(node.value)) {
        const auto v = std::get<i32>(node.value);
        _last_value = "imm.i32 " + std::to_string(v);

    } else if (std::holds_alternative<f32>(node.value)) {
        _last_value = "imm.f32 " + std::to_string(std::get<f32>(node.value));

    } else if (std::holds_alternative<std::string_view>(node.value)) {
        _last_value = "const.str \"" + std::string(std::get<std::string_view>(node.value)) + "\"";

    } else if (std::holds_alternative<bool>(node.value)) {
        _last_value = (std::get<bool>(node.value) ? "imm.bool 1" : "imm.bool 0");
    }
}

void cplus::ir::IntermediateRepresentation::visit(ast::IdentifierExpression &node)
{
    const auto it = _value_map.find(std::string(node.name));

    if (it != _value_map.end()) {
        _last_value = it->second;
    } else {
        _last_value = std::string(node.name);
    }
}

void cplus::ir::IntermediateRepresentation::visit(ast::BinaryExpression &node)
{
    node.left->accept(*this);
    const std::string left = _last_value;

    node.right->accept(*this);
    const std::string right = _last_value;

    const std::string op = binary_op_to_string(node.op);
    const std::string tmp = new_temp("t");

    emit("  " + tmp + " = " + op + " " + left + ", " + right);
    _last_value = tmp;
}

void cplus::ir::IntermediateRepresentation::visit(ast::UnaryExpression &node)
{
    node.operand->accept(*this);

    const std::string src = _last_value;
    const std::string opstr = unary_op_to_string(node.op);
    const std::string tmp = new_temp("u");

    switch (node.op) {
        case ast::UnaryExpression::NOT:
            emit("  " + tmp + " = icmp.eq " + src + ", const." + ast::to_string(node.type->kind) + "0");
            break;
        case ast::UnaryExpression::NEGATE:

            emit("  " + tmp + " = neg " + src);
            break;
        case ast::UnaryExpression::INC:
            emit("  " + tmp + " = add " + src + ", const." + ast::to_string(node.type->kind) + "1");
            break;
        case ast::UnaryExpression::DEC:
            emit("  " + tmp + " = sub " + src + ", const." + ast::to_string(node.type->kind) + "1");
            break;
        default:
            emit("  " + tmp + " = " + opstr + " " + src);
            break;
    }
    _last_value = tmp;
}

void cplus::ir::IntermediateRepresentation::visit(ast::CallExpression &node)
{
    std::vector<std::string> args;
    args.reserve(node.arguments.size());

    for (const auto &arg : node.arguments) {
        arg->accept(*this);
        args.push_back(_last_value);
    }

    const std::string tmp = new_temp("call");
    std::string arglist;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i)
            arglist += ", ";
        arglist += args[i];
    }

    emit("  " + tmp + " = call @" + std::string(node.function_name) + "(" + arglist + ")");
    _last_value = tmp;
}

void cplus::ir::IntermediateRepresentation::visit(ast::AssignmentExpression &node)
{
    node.value->accept(*this);

    const std::string value = _last_value;
    const std::string vname(node.variable_name);
    const std::string ssa = new_temp(vname);

    emit("  " + ssa + " = mov " + value);
    _last_value = ssa;
}

void cplus::ir::IntermediateRepresentation::visit(ast::ExpressionStatement &node)
{
    if (node.expression) {
        node.expression->accept(*this);
    }
    _last_value.clear();
}

void cplus::ir::IntermediateRepresentation::visit(ast::BlockStatement &node)
{
    for (const auto &stmt : node.statements) {
        stmt->accept(*this);
    }
}

void cplus::ir::IntermediateRepresentation::visit(ast::VariableDeclaration &node)
{
    const std::string name(node.name);
    const std::string ssa = new_temp(name);

    if (node.initializer) {
        node.initializer->accept(*this);
        emit("  " + ssa + " = mov " + _last_value);
        _last_value.clear();
    } else {
        emit("  " + ssa + " = undef");
    }
    _value_map[name] = ssa;
}

void cplus::ir::IntermediateRepresentation::visit(ast::ReturnStatement &node)
{
    if (node.value) {
        node.value->accept(*this);
        emit("  ret " + _last_value);

    } else {
        emit("  ret");
    }

    _last_value.clear();
}

void cplus::ir::IntermediateRepresentation::visit(ast::IfStatement __attribute__((unused)) & node)
{
    //TODO
}

void cplus::ir::IntermediateRepresentation::visit(ast::ForStatement __attribute__((unused)) & node)
{
    //TODO
}

void cplus::ir::IntermediateRepresentation::visit(ast::ForeachStatement __attribute__((unused)) & node)
{
    //TODO
}

void cplus::ir::IntermediateRepresentation::visit(ast::CaseStatement __attribute__((unused)) & node)
{
    //TODO
}

void cplus::ir::IntermediateRepresentation::visit(ast::FunctionDeclaration &node)
{
    _current_function = std::string(node.name);
    emit("func @" + _current_function + "() -> " + ast::to_string(node.return_type->kind));
    emit("{");

    for (u64 i = 0; i < node.parameters.size(); ++i) {
        const auto &p = node.parameters[i];
        const std::string name(p.name);
        const std::string ssa = new_temp(name);

        emit("  " + ssa + " = arg " + std::to_string(i));
        _value_map[name] = ssa;
    }

    if (node.body) {
        node.body->accept(*this);
    }

    /** @brief only emit implicit return if last statement wasn’t a return */
    if (_output.rfind("  ret", _output.size() - 5) == std::string::npos) {
        emit("  ret");
    }

    emit("}");
}

void cplus::ir::IntermediateRepresentation::visit(ast::Module &node)
{
    for (const auto &decl : node.declarations) {
        decl->accept(*this);
    }
}
