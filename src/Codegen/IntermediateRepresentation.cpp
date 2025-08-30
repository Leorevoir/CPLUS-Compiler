#include "CPlus/Arguments.hpp"
#include <CPlus/Codegen/IntermediateRepresentation.hpp>
#include <CPlus/Logger.hpp>

#include <unordered_set>

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
    _value_map_stack.clear();

    if (cplus_flags & FLAG_DEBUG) {
        logger::info("IntermediateRepresentation::run", "Generating IR for module " + module->name);
    }

    _push();
    _emit("; C+ generated IR for module " + module->name);
    module->accept(*this);
    _pop();

    if (_value_map_stack.size() != 0) {
        throw exception::Error("IntermediateRepresentation::run", "value map stack not empty after processing module");
    }

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

void cplus::ir::IntermediateRepresentation::_emit(const std::string &s)
{
    //TODO: use a logger for this to highlight IR output
    if (cplus_flags & FLAG_SHOW_IR) {
        std::cout << s << std::endl;
    }

    _output += s;
    _output += '\n';
}

std::string cplus::ir::IntermediateRepresentation::_new_temp(const std::string &hint)
{
    return "%" + hint + std::to_string(_temp_counter++);
}

std::string cplus::ir::IntermediateRepresentation::_new_label(const std::string &hint)
{
    return hint + std::to_string(_label_counter++);
}

/**
 * scopes
 */

inline std::unordered_map<std::string, std::string> &cplus::ir::IntermediateRepresentation::_current_map()
{
    if (_value_map_stack.empty()) {
        _value_map_stack.emplace_back();
    }
    return _value_map_stack.back();
}

inline void cplus::ir::IntermediateRepresentation::_push_copy()
{
    if (_value_map_stack.empty()) {
        _value_map_stack.emplace_back();
    } else {
        _value_map_stack.push_back(_current_map());
    }
}

inline void cplus::ir::IntermediateRepresentation::_push()
{
    _value_map_stack.emplace_back();
}

inline void cplus::ir::IntermediateRepresentation::_pop()
{
    if (!_value_map_stack.empty()) {
        _value_map_stack.pop_back();
    }
}

std::string cplus::ir::IntermediateRepresentation::_lookup(const std::string &name) const
{
    for (auto it = _value_map_stack.rbegin(); it != _value_map_stack.rend(); ++it) {
        const auto &m = *it;
        const auto found = m.find(name);

        if (found != m.end()) {
            return found->second;
        }
    }

    /** @brief should never happen bc of semantic analysis (../Analysis/SymbolTable.cpp) */
    return name;
}

inline void cplus::ir::IntermediateRepresentation::_set_name(const std::string &name, const std::string &ssa)
{
    _current_map()[name] = ssa;
}

/**
* AST visitor
*/

/**
* @brief literal expression
* @note handles i32, f32, bool, and string literals but i dont like this implementation
*/
void cplus::ir::IntermediateRepresentation::visit(ast::LiteralExpression &node)
{
    //TODO: find a better way to handle different literal types
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

/**
 * @brief identifier expression
 * @note looks up current SSA mapping for the identifier
 */
void cplus::ir::IntermediateRepresentation::visit(ast::IdentifierExpression &node)
{
    const std::string name(node.name);
    const std::string found = _lookup(name);

    _last_value = found;
}

void cplus::ir::IntermediateRepresentation::visit(ast::BinaryExpression &node)
{
    node.left->accept(*this);
    const std::string left = _last_value;

    node.right->accept(*this);
    const std::string right = _last_value;

    const std::string op = binary_op_to_string(node.op);
    const std::string tmp = _new_temp("t");

    _emit("  " + tmp + " = " + op + " " + left + ", " + right);
    _last_value = tmp;
}

/**
* @brief unary expression
* @note ++ -- ! - (type should be handled in semantic analysis "../Analysis/SymbolTable.cpp")
*/
void cplus::ir::IntermediateRepresentation::visit(ast::UnaryExpression &node)
{
    //TODO: refactor this
    const bool operand_is_ident = dynamic_cast<ast::IdentifierExpression *>(node.operand.get()) != nullptr;
    std::string ident_name;

    if (operand_is_ident) {
        const auto *ident = dynamic_cast<ast::IdentifierExpression *>(node.operand.get());

        if (ident) {
            ident_name = std::string(ident->name);
        }
    }

    node.operand->accept(*this);
    const std::string src = _last_value;
    const std::string tmp = _new_temp("u");

    switch (node.op) {
        case ast::UnaryExpression::NOT:
            _emit("  " + tmp + " = icmp.eq " + src + ", const." + ast::to_string(node.type->kind) + "0");
            break;
        case ast::UnaryExpression::NEGATE:
            _emit("  " + tmp + " = neg " + src);
            break;
        case ast::UnaryExpression::INC:
            _emit("  " + tmp + " = add " + src + ", const." + ast::to_string(node.type->kind) + "1");
            break;
        case ast::UnaryExpression::DEC:
            _emit("  " + tmp + " = sub " + src + ", const." + ast::to_string(node.type->kind) + "1");
            break;
        default:
            _emit("  " + tmp + " = " + unary_op_to_string(node.op) + " " + src);
            break;
    }

    /** @brief update current mapping to the new SSA for the identifier only if modifying */
    if (operand_is_ident && !ident_name.empty() && (node.op == ast::UnaryExpression::INC || node.op == ast::UnaryExpression::DEC)) {
        _set_name(ident_name, tmp);
    }

    _last_value = tmp;
}

/**
* @brief function call expression
* @note arguments are evaluated left to right
*/
void cplus::ir::IntermediateRepresentation::visit(ast::CallExpression &node)
{
    std::vector<std::string> args;
    args.reserve(node.arguments.size());

    for (const auto &arg : node.arguments) {
        arg->accept(*this);
        args.push_back(_last_value);
    }

    const std::string tmp = _new_temp("call");
    std::string arglist;

    for (u64 i = 0; i < args.size(); ++i) {
        if (i) {
            arglist += ", ";
        }
        arglist += args[i];
    }

    _emit("  " + tmp + " = call @" + std::string(node.function_name) + "(" + arglist + ")");
    _last_value = tmp;
}

/**
* @brief assignment expression
* @note variable must already be declared (checked in semantic analysis "../Analysis/SymbolTable.cpp")
*/
void cplus::ir::IntermediateRepresentation::visit(ast::AssignmentExpression &node)
{
    node.value->accept(*this);

    const std::string value = _last_value;
    const std::string vname(node.variable_name);
    const std::string ssa = _new_temp(vname);

    _emit("  " + ssa + " = mov " + value);
    _set_name(vname, ssa);
    _last_value = ssa;
}

/**
* @brief expression statement
* @note TODO
*/
void cplus::ir::IntermediateRepresentation::visit(ast::ExpressionStatement &node)
{
    if (node.expression) {
        node.expression->accept(*this);
    }
    _last_value.clear();
}

/**
* @brief block statement
* @note creates a new scope for variables
*/
void cplus::ir::IntermediateRepresentation::visit(ast::BlockStatement &node)
{
    _push_copy();
    for (const auto &stmt : node.statements) {
        stmt->accept(*this);
    }
    _pop();
}

/**
* @brief variable declaration
* @note initializer is optional
*/
void cplus::ir::IntermediateRepresentation::visit(ast::VariableDeclaration &node)
{
    const std::string name(node.name);
    const std::string ssa = _new_temp(name);

    if (node.initializer) {
        node.initializer->accept(*this);
        _emit("  " + ssa + " = mov " + _last_value);
        _last_value.clear();
    } else {
        _emit("  " + ssa + " = undef");
    }
    _set_name(name, ssa);
}

/**
* @brief return statement
* @note value is optional bc we've already checked for void functions in semantic analysis
*/
void cplus::ir::IntermediateRepresentation::visit(ast::ReturnStatement &node)
{
    if (node.value) {
        node.value->accept(*this);
        _emit("  ret " + _last_value);

    } else {
        _emit("  ret");
    }

    // _last_value.clear();
}

/**
 * @brief if-else statement handling with SSA phi nodes
 * @note cond is evaluated first, but branch labels are not emitted here to keep IR structured
 */
void cplus::ir::IntermediateRepresentation::visit(ast::IfStatement &node)
{
    /** @brief evaluate condition first */
    node.condition->accept(*this);
    const std::string cond = _last_value;
    _last_value.clear();

    const std::string then_label = _new_label("if.then");
    const std::string else_label = node.else_statement ? _new_label("if.else") : _new_label("if.end");
    const std::string end_label = _new_label("if.end");

    _emit("  br " + cond + ", %" + then_label + ", %" + else_label);

    /** @brief snapshot parent map */
    const std::unordered_map<std::string, std::string> parent_map = _current_map();

    /** @brief then branch */
    _emit("label %" + then_label + ":");
    _push_copy();
    if (node.then_statement) {
        node.then_statement->accept(*this);
    }
    const std::unordered_map<std::string, std::string> then_map = _current_map();
    _pop();
    _emit("  br %" + end_label);

    /** @brief else branch (if any) */
    std::unordered_map<std::string, std::string> else_map;
    _emit("label %" + else_label + ":");
    if (node.else_statement) {
        _push_copy();
        node.else_statement->accept(*this);
        else_map = _current_map();
        _pop();
    } else {
        else_map = parent_map;
    }
    _emit("  br %" + end_label);

    /** @brief end label and phis */
    _emit("label %" + end_label + ":");

    /** @brief collect all variable names from parent, then, and else maps */
    std::unordered_set<std::string> varset;
    for (const auto &kv : parent_map) {
        varset.insert(kv.first);
    }
    for (const auto &kv : then_map) {
        varset.insert(kv.first);
    }
    for (const auto &kv : else_map) {
        varset.insert(kv.first);
    }

    /** @brief foreach variable with differing results, emit a phi & update current mapping */
    for (const auto &var : varset) {
        const std::string parent_ssa = parent_map.count(var) ? parent_map.at(var) : "undef";
        const std::string then_ssa = then_map.count(var) ? then_map.at(var) : parent_ssa;
        const std::string else_ssa = else_map.count(var) ? else_map.at(var) : parent_ssa;

        if (then_ssa == else_ssa) {
            _set_name(var, then_ssa);
            continue;
        }

        const std::string phi_ssa = _new_temp(var + "_phi");

        _emit("  " + phi_ssa + " = phi [" + then_ssa + ", %" + then_label + "], [" + else_ssa + ", %" + else_label + "]");
        _set_name(var, phi_ssa);
    }
}

/**
 * @brief for loop
 * @note TODO
 */
void cplus::ir::IntermediateRepresentation::visit(ast::ForStatement __attribute__((unused)) & node)
{
    //TODO
}

/**
* @brief foreach loop
* @note TODO
*/
void cplus::ir::IntermediateRepresentation::visit(ast::ForeachStatement __attribute__((unused)) & node)
{
    //TODO
}

/**
* @brief case statement
* @note TODO
 */
void cplus::ir::IntermediateRepresentation::visit(ast::CaseStatement __attribute__((unused)) & node)
{
    //TODO
}

/**
* @brief function declaration
* @note creates a new scope for parameters and local variables
*/
void cplus::ir::IntermediateRepresentation::visit(ast::FunctionDeclaration &node)
{
    _current_function = std::string(node.name);
    _emit("func @" + _current_function + "() -> " + ast::to_string(node.return_type->kind));
    _emit("{");

    _push();

    for (u64 i = 0; i < node.parameters.size(); ++i) {
        const auto &p = node.parameters[i];
        const std::string name(p.name);
        const std::string ssa = _new_temp(name);

        _emit("  " + ssa + " = arg " + std::to_string(i));
        _set_name(name, ssa);
    }

    if (node.body) {
        node.body->accept(*this);
    }

    /** @brief only emit implicit return if last statement wasn’t a return to avoid multiple ret */
    if (_output.rfind("  ret", _output.size() - 5) == std::string::npos) {
        _emit("  ret");
    }

    _emit("}");
    _pop();
}

/**
* @brief module
* @note ast visitor entry point
*/
void cplus::ir::IntermediateRepresentation::visit(ast::Module &node)
{
    for (const auto &decl : node.declarations) {
        decl->accept(*this);
    }
}
