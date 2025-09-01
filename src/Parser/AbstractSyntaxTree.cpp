#include <CPlus/Arguments.hpp>
#include <CPlus/Parser/AbstractSyntaxTree.hpp>
#include <CPlus/Parser/Logger.hpp>

/**
* public
*/

std::unique_ptr<cplus::ast::Module> cplus::ast::AbstractSyntaxTree::run(const std::vector<lx::Token> &tokens)
{
    _tokens = tokens;
    _current = 0;

    auto module = _parse_module();

    if (cplus_flags & FLAG_SHOW_AST) {
        ASTLogger logger;
        logger.show(*module);
    }

    return module;
}

/**
 * helpers
 */

bool cplus::ast::AbstractSyntaxTree::_is_at_end() const
{
    if (_current >= _tokens.size()) {
        return true;
    }
    return _tokens[_current].kind == lx::TokenKind::TOKEN_EOF;
}

const cplus::lx::Token &cplus::ast::AbstractSyntaxTree::_peek() const
{
    if (_current >= _tokens.size()) {
        static const lx::Token eof_token{lx::TokenKind::TOKEN_EOF, "", 0, 0};
        return eof_token;
    }
    return _tokens[_current];
}

const cplus::lx::Token &cplus::ast::AbstractSyntaxTree::_previous() const
{
    if (_current == 0) {
        static const lx::Token start_token{lx::TokenKind::TOKEN_EOF, "", 0, 0};
        return start_token;
    }
    return _tokens[_current - 1];
}

const cplus::lx::Token &cplus::ast::AbstractSyntaxTree::_advance()
{
    if (!_is_at_end()) {
        ++_current;
    }
    return _previous();
}

const cplus::lx::Token &cplus::ast::AbstractSyntaxTree::_consume(lx::TokenKind kind, const std::string &message)
{
    if (_check(kind)) {
        return _advance();
    }

    const auto &current = _peek();
    throw exception::Error("AbstractSyntaxTree::_consume", message, " in module: ", _module, " at ", std::to_string(current.line), ":",
        std::to_string(current.column));
}

bool cplus::ast::AbstractSyntaxTree::_check(lx::TokenKind kind) const
{
    if (_current >= _tokens.size()) {
        return false;
    }
    return _tokens[_current].kind == kind;
}

bool cplus::ast::AbstractSyntaxTree::_check(lx::TokenKind kind, u64 offset) const
{
    if (_current + offset >= _tokens.size()) {
        return false;
    }
    return _tokens[_current + offset].kind == kind;
}

bool cplus::ast::AbstractSyntaxTree::_match(const std::initializer_list<lx::TokenKind> &kinds)
{
    for (const auto &kind : kinds) {
        if (_check(kind)) {
            _advance();
            return true;
        }
    }
    return false;
}

void cplus::ast::AbstractSyntaxTree::_synchronize()
{
    while (!_is_at_end()) {
        if (_previous().kind == lx::TokenKind::TOKEN_SEMICOLON || _peek().kind == lx::TokenKind::TOKEN_CLOSE_BRACE) {
            return;
        }
        switch (_peek().kind) {
            case lx::TokenKind::TOKEN_DEF:
            case lx::TokenKind::TOKEN_CONST:
            case lx::TokenKind::TOKEN_IF:
            case lx::TokenKind::TOKEN_FOR:
            case lx::TokenKind::TOKEN_FOREACH:
            case lx::TokenKind::TOKEN_CASE:
            case lx::TokenKind::TOKEN_RETURN:
                return;
            default:
                _advance();
        }
    }
}

/**
* private
*/

std::unique_ptr<cplus::ast::Module> cplus::ast::AbstractSyntaxTree::_parse_module()
{
    auto module = make<Module>();
    const auto module_name = _consume(lx::TokenKind::TOKEN_MODULE, "Lexical error, expected 'module'");

    module->name = std::move(module_name.lexeme);
    _module = module->name.c_str();

    logger::info("Building AST for module: ", _module, "...");

    while (!_is_at_end()) {
        if (auto decl = _parse_declaration()) {
            module->declarations.push_back(std::move(decl));
        }
    }
    return module;
}

cplus::ast::StatementPtr cplus::ast::AbstractSyntaxTree::_parse_declaration()
{
    try {

        if (_match({lx::TokenKind::TOKEN_DEF})) {
            return _parse_function_declaration();
        }
        if (_match({lx::TokenKind::TOKEN_CONST})) {
            return _parse_variable_declaration(true, true);
        }

        return _parse_statement();

    } catch (const std::exception &e) {

        _synchronize();
        throw exception::Error("AbstractSyntaxTree::_parse_declaration", e.what());
    }
}

/**
* @brief function declaration
* @syntax def function_name(param:type, ...) -> return_type { body };
*
* def add(a: int, b: int) -> int
* {
*     return a + b;
* }
*
* def main() -> int
* {
*     return add(5, 10);
* }
*/
cplus::ast::StatementPtr cplus::ast::AbstractSyntaxTree::_parse_function_declaration()
{
    const auto name = _consume(lx::TokenKind::TOKEN_IDENTIFIER, "Expected function name");
    auto func = make<FunctionDeclaration>(name.lexeme);

    func->line = name.line;
    func->column = name.column;

    _consume(lx::TokenKind::TOKEN_OPEN_PAREN, "Expected '(' after function name");

    if (!_check(lx::TokenKind::TOKEN_CLOSE_PAREN)) {

        do {
            const auto param_name = _consume(lx::TokenKind::TOKEN_IDENTIFIER, "Expected parameter name");

            TypePtr param_type = nullptr;
            if (_match({lx::TokenKind::TOKEN_COLON})) {
                param_type = _parse_type();
            }

            func->parameters.emplace_back(param_name.lexeme, std::move(param_type));
        } while (_match({lx::TokenKind::TOKEN_COMMA}));
    }

    _consume(lx::TokenKind::TOKEN_CLOSE_PAREN, "Expected ')' after parameters");

    if (_match({lx::TokenKind::TOKEN_ARROW})) {
        func->return_type = _parse_type();
    }

    func->body = _parse_block_statement();
    return func;
}

/**
* @brief variable declaration
* @syntax var_name: type;
* @syntax var_name: type = initializer;
* @syntax var_name = initializer;
*
* x: int;
* x:int = 10;
* x = 10;
*
* @note type is inferred from initializer if not explicitly provided
*/
cplus::ast::StatementPtr cplus::ast::AbstractSyntaxTree::_parse_variable_declaration(bool is_const, bool expect_semicolon = true)
{
    const auto name = _consume(lx::TokenKind::TOKEN_IDENTIFIER, "Expected variable name");
    auto var_decl = make<VariableDeclaration>(name.lexeme, is_const);

    var_decl->line = name.line;
    var_decl->column = name.column;

    if (_match({lx::TokenKind::TOKEN_COLON})) {
        var_decl->declared_type = _parse_type();
    }
    if (_match({lx::TokenKind::TOKEN_ASSIGN})) {
        var_decl->initializer = _parse_expression();
    }

    if (expect_semicolon) {
        _consume(lx::TokenKind::TOKEN_SEMICOLON, "Expected ';' after variable declaration");
    }
    return var_decl;
}

cplus::ast::TypePtr cplus::ast::AbstractSyntaxTree::_parse_type()
{
    const auto type_token = _consume(lx::TokenKind::TOKEN_IDENTIFIER, "Expected type name");
    Type::Kind kind = from_string(type_token.lexeme);

    return make<Type>(kind, type_token.lexeme);
}

/**
* @brief statement
* @syntax
*   expression_statement
*   if_statement
*   for_statement
*   foreach_statement
*   case_statement
*   return_statement
*   block_statement
*   variable_declaration
*
* statement:
*   expression;
*
* if (condition) { then_stmt } else { else_stmt }
*
* for (initializer; condition; increment) { body }
*
* foreach (iterator in iterable) { body }
*
* case (expression) {
*     value1: statements;
*     value2: statements;
*     ...
*     default: statements;
* }
*
* return expression;
*
* {
*   statement1;
*   statement2;
*   ...
* }
*
* var_name: type = initializer;
*
*/
cplus::ast::StatementPtr cplus::ast::AbstractSyntaxTree::_parse_statement()
{
    if (_match({lx::TokenKind::TOKEN_IF})) {
        return _parse_if_statement();
    }
    if (_match({lx::TokenKind::TOKEN_FOR})) {
        return _parse_for_statement();
    }
    if (_match({lx::TokenKind::TOKEN_FOREACH})) {
        return _parse_foreach_statement();
    }
    if (_match({lx::TokenKind::TOKEN_CASE})) {
        return _parse_case_statement();
    }
    if (_match({lx::TokenKind::TOKEN_RETURN})) {
        return _parse_return_statement();
    }
    if (_check(lx::TokenKind::TOKEN_OPEN_BRACE)) {
        return _parse_block_statement();
    }
    if (_check(lx::TokenKind::TOKEN_IDENTIFIER)) {
        const u64 pos = _current;
        _advance();
        if (_check(lx::TokenKind::TOKEN_COLON) || _check(lx::TokenKind::TOKEN_ASSIGN)) {
            _current = pos;
            return _parse_variable_declaration(false, true);
        }
        _current = pos;
        return _parse_expression_statement();
    }
    return _parse_expression_statement();
}

/**
 * @brief block statement
 * @syntax { statement* }
 *
 * {
 *   statement1;
 *   statement2;
 *   ...
 * }
 */
cplus::ast::StatementPtr cplus::ast::AbstractSyntaxTree::_parse_block_statement()
{
    _consume(lx::TokenKind::TOKEN_OPEN_BRACE, "Expected '{'");

    auto block = make<BlockStatement>();

    while (!_check(lx::TokenKind::TOKEN_CLOSE_BRACE) && !_is_at_end()) {

        if (auto stmt = _parse_declaration()) {
            block->statements.push_back(std::move(stmt));
        }
    }

    _consume(lx::TokenKind::TOKEN_CLOSE_BRACE, "Expected '}'");
    return block;
}

/**
 * @brief if statement
 * @syntax if (condition) { then_stmt } else { else_stmt }
 *
 * if (x > 0) {
 *     print("positive");
 * } else {
 *     print("negative");
 * }
 */
cplus::ast::StatementPtr cplus::ast::AbstractSyntaxTree::_parse_if_statement()
{
    while (_match({lx::TokenKind::TOKEN_OPEN_PAREN}) && _check(lx::TokenKind::TOKEN_CLOSE_PAREN))
        ;
    auto condition = _parse_expression();
    while (_match({lx::TokenKind::TOKEN_CLOSE_PAREN}) && _check(lx::TokenKind::TOKEN_CLOSE_PAREN))
        ;

    auto then_stmt = _parse_statement();
    auto if_stmt = make<IfStatement>(std::move(condition), std::move(then_stmt));

    if (_match({lx::TokenKind::TOKEN_ELSE})) {
        if_stmt->else_statement = _parse_statement();
    }

    return if_stmt;
}

/**
* @brief for loop statement
* @syntax for (initializer; condition; increment) { body }
* @syntax for initializer; condition; increment { body }
*
* for (i:int = 0; i < 10; ++i) {
*     print(i);
* }
*
* for i = 0; i < 10; ++i {
*     print(i);
* }
*
* @note the parentheses are sugar syntax, they can be omitted if not needed
*/
cplus::ast::StatementPtr cplus::ast::AbstractSyntaxTree::_parse_for_statement()
{
    auto for_stmt = make<ForStatement>();
    const bool has_paren = _match({lx::TokenKind::TOKEN_OPEN_PAREN});

    if (!_check(lx::TokenKind::TOKEN_SEMICOLON)) {

        if (_check(lx::TokenKind::TOKEN_IDENTIFIER)) {
            const u64 pos = _current;
            _advance();

            if (_check(lx::TokenKind::TOKEN_COLON) || _check(lx::TokenKind::TOKEN_ASSIGN)) {
                _current = pos;
                for_stmt->initializer = _parse_variable_declaration(false, false);
            } else {
                _current = pos;
                for_stmt->initializer = make<ExpressionStatement>(_parse_expression());
            }

        } else {
            for_stmt->initializer = make<ExpressionStatement>(_parse_expression());
        }
    }

    _consume(lx::TokenKind::TOKEN_SEMICOLON, "Expected ';' after for loop initializer");

    if (!_check(lx::TokenKind::TOKEN_SEMICOLON)) {
        for_stmt->condition = _parse_expression();
    }

    _consume(lx::TokenKind::TOKEN_SEMICOLON, "Expected ';' after for loop condition");

    if (!_check(lx::TokenKind::TOKEN_OPEN_BRACE) && !(has_paren && _check(lx::TokenKind::TOKEN_CLOSE_PAREN))) {
        for_stmt->increment = _parse_expression();
    }

    if (has_paren) {
        _consume(lx::TokenKind::TOKEN_CLOSE_PAREN, "Expected ')' after for loop increment");
    }

    for_stmt->body = _parse_statement();

    return for_stmt;
}

/**
* @brief foreach loop statement
* @syntax foreach (iterator in iterable) { body }
* @syntax foreach iterator in iterable { body }
*
* foreach (item in collection) {
*     print(item);
* }
*
* foreach c in "Hello C+" {
*     print(c);
* }
*
* better than for (i = 0; i < collection.size(); ++i) {print(collection[i]);}
*/
cplus::ast::StatementPtr cplus::ast::AbstractSyntaxTree::_parse_foreach_statement()
{
    const bool has_paren = _match({lx::TokenKind::TOKEN_OPEN_PAREN});
    const auto iterator_token = _consume(lx::TokenKind::TOKEN_IDENTIFIER, "Expected iterator name in foreach");

    _consume(lx::TokenKind::TOKEN_IN, "Expected 'in' after iterator in foreach");

    auto iterable = _parse_expression();

    if (has_paren) {
        _consume(lx::TokenKind::TOKEN_CLOSE_PAREN, "Expected ')' after foreach expression");
    }

    auto body = _parse_statement();

    return make<ForeachStatement>(iterator_token.lexeme, std::move(iterable), std::move(body));
}

/**
* @brief case statement
* @syntax case (expression) { value1: statements; value2: statements; ... default: statements; }
*
* case (x) {
*   1: print("one");
*   2: print("two");
*   default: print("other");
* }
*
* like a switch case, this is faster than multiple if-else if statements
*/
cplus::ast::StatementPtr cplus::ast::AbstractSyntaxTree::_parse_case_statement()
{
    _consume(lx::TokenKind::TOKEN_OPEN_PAREN, "Expected '(' after 'case'");
    auto expression = _parse_expression();
    _consume(lx::TokenKind::TOKEN_CLOSE_PAREN, "Expected ')' after case expression");
    _consume(lx::TokenKind::TOKEN_OPEN_BRACE, "Expected '{' before case clauses");

    auto case_stmt = make<CaseStatement>(std::move(expression));

    while (!_check(lx::TokenKind::TOKEN_CLOSE_BRACE) && !_is_at_end()) {
        CaseStatement::CaseClause clause;
        clause.value = _match({lx::TokenKind::TOKEN_DEFAULT}) ? nullptr : _parse_expression();
        _consume(lx::TokenKind::TOKEN_COLON, "Expected ':' after case value");

        while (!_check(lx::TokenKind::TOKEN_CLOSE_BRACE) && !_is_at_end()
            && !(_check(lx::TokenKind::TOKEN_INTEGER) || _check(lx::TokenKind::TOKEN_DEFAULT))) {
            if (auto stmt = _parse_declaration()) {
                clause.statements.push_back(std::move(stmt));
            }
        }
        case_stmt->clauses.push_back(std::move(clause));
    }

    _consume(lx::TokenKind::TOKEN_CLOSE_BRACE, "Expected '}' after case clauses");
    return case_stmt;
}

/**
* @brief parse return statement
* @syntax return expression;
*
* def main() ->int
* {
*     return 84;
* }
*/
cplus::ast::StatementPtr cplus::ast::AbstractSyntaxTree::_parse_return_statement()
{
    cplus::ast::ExpressionPtr value = nullptr;

    if (!_check(lx::TokenKind::TOKEN_SEMICOLON)) {
        value = _parse_expression();
    }

    _consume(lx::TokenKind::TOKEN_SEMICOLON, "Expected ';' after return value");

    return make<ReturnStatement>(std::move(value));
}

cplus::ast::StatementPtr cplus::ast::AbstractSyntaxTree::_parse_expression_statement()
{
    auto expr = _parse_expression();
    _consume(lx::TokenKind::TOKEN_SEMICOLON, "Expected ';' after expression");

    return make<ExpressionStatement>(std::move(expr));
}

cplus::ast::ExpressionPtr cplus::ast::AbstractSyntaxTree::_parse_expression()
{
    return _parse_logical_or();
}

/**
* @brief parse logical OR
* @syntax expression || expression
*
* if (a > 0 || b < 0) { ... }
*/
cplus::ast::ExpressionPtr cplus::ast::AbstractSyntaxTree::_parse_logical_or()
{
    auto expr = _parse_logical_and();

    while (_match({lx::TokenKind::TOKEN_CMP_OR})) {
        auto op = BinaryExpression::OR;
        auto right = _parse_logical_and();
        expr = make<BinaryExpression>(std::move(expr), op, std::move(right));
    }

    return expr;
}

/**
* @brief parse logical AND
* @syntax expression && expression
*
* if (a > 0 && b < 0) { ... }
*/
cplus::ast::ExpressionPtr cplus::ast::AbstractSyntaxTree::_parse_logical_and()
{
    auto expr = _parse_equality();

    while (_match({lx::TokenKind::TOKEN_CMP_AND})) {
        auto op = BinaryExpression::AND;
        auto right = _parse_equality();
        expr = make<BinaryExpression>(std::move(expr), op, std::move(right));
    }

    return expr;
}

/**
* @brief parse equality
* @syntax expression == expression
* @syntax expression != expression
*
* if (expression == expression) { ... }
* if (expression != expression) { ... }
*/
cplus::ast::ExpressionPtr cplus::ast::AbstractSyntaxTree::_parse_equality()
{
    auto expr = _parse_comparison();

    while (_match({lx::TokenKind::TOKEN_EQ, lx::TokenKind::TOKEN_NEQ})) {
        auto op = (_previous().kind == lx::TokenKind::TOKEN_EQ) ? BinaryExpression::EQ : BinaryExpression::NEQ;
        auto right = _parse_comparison();
        expr = make<BinaryExpression>(std::move(expr), op, std::move(right));
    }

    return expr;
}

/**
* @brief parse comparison
* @syntax expression > expression
* @syntax expression >= expression
* @syntax expression < expression
* @syntax expression <= expression
*
* if (a > b) { ... }
* if (a >= b) { ... }
* if (a < b) { ... }
* if (a <= b) { ... }
*/
cplus::ast::ExpressionPtr cplus::ast::AbstractSyntaxTree::_parse_comparison()
{
    auto expr = _parse_term();

    while (_match({lx::TokenKind::TOKEN_GT, lx::TokenKind::TOKEN_GTE, lx::TokenKind::TOKEN_LT, lx::TokenKind::TOKEN_LTE})) {
        BinaryExpression::Operator op;
        switch (_previous().kind) {
            case lx::TokenKind::TOKEN_GT:
                op = BinaryExpression::GT;
                break;
            case lx::TokenKind::TOKEN_GTE:
                op = BinaryExpression::GTE;
                break;
            case lx::TokenKind::TOKEN_LT:
                op = BinaryExpression::LT;
                break;
            case lx::TokenKind::TOKEN_LTE:
                op = BinaryExpression::LTE;
                break;
            default:
                throw exception::Error("AbstractSyntaxTree::_parse_comparison", "Unknown comparison operator at ",
                    std::to_string(_previous().line), ":", std::to_string(_previous().column));
                break;
        }

        auto right = _parse_term();
        expr = make<BinaryExpression>(std::move(expr), op, std::move(right));
    }

    return expr;
}

/**
* @brief parse term
* @syntax expression + expression
* @syntax expression - expression
*
* a + b - c
*/
cplus::ast::ExpressionPtr cplus::ast::AbstractSyntaxTree::_parse_term()
{
    auto expr = _parse_factor();

    while (_match({lx::TokenKind::TOKEN_MINUS, lx::TokenKind::TOKEN_PLUS})) {
        auto op = (_previous().kind == lx::TokenKind::TOKEN_MINUS) ? BinaryExpression::SUB : BinaryExpression::ADD;
        auto right = _parse_factor();
        expr = make<BinaryExpression>(std::move(expr), op, std::move(right));
    }

    return expr;
}

/**
* @brief parse factor
* @syntax expression * expression
* @syntax expression / expression
* @syntax expression % expression
*
* if (a * b / c % d) { ... }
*/
cplus::ast::ExpressionPtr cplus::ast::AbstractSyntaxTree::_parse_factor()
{
    auto expr = _parse_unary();

    while (_match({lx::TokenKind::TOKEN_SLASH, lx::TokenKind::TOKEN_ASTERISK, lx::TokenKind::TOKEN_MODULO})) {
        BinaryExpression::Operator op;
        switch (_previous().kind) {
            case lx::TokenKind::TOKEN_SLASH:
                op = BinaryExpression::DIV;
                break;
            case lx::TokenKind::TOKEN_ASTERISK:
                op = BinaryExpression::MUL;
                break;
            case lx::TokenKind::TOKEN_MODULO:
                op = BinaryExpression::MOD;
                break;
            default:
                throw exception::Error("AbstractSyntaxTree::_parse_factor", "Unknown factor operator at ", std::to_string(_previous().line),
                    ":", std::to_string(_previous().column));
                break;
        }

        auto right = _parse_unary();
        expr = make<BinaryExpression>(std::move(expr), op, std::move(right));
    }

    return expr;
}

/**
* @brief parse unary
* @syntax -expression
* @syntax !expression
* @syntax +expression
*/
cplus::ast::ExpressionPtr cplus::ast::AbstractSyntaxTree::_parse_unary()
{
    if (_match({lx::TokenKind::TOKEN_CMP_NOT, lx::TokenKind::TOKEN_MINUS, lx::TokenKind::TOKEN_PLUS, lx::TokenKind::TOKEN_INC,
            lx::TokenKind::TOKEN_DEC})) {
        UnaryExpression::Operator op;
        switch (_previous().kind) {
            case lx::TokenKind::TOKEN_CMP_NOT:
                op = UnaryExpression::NOT;
                break;
            case lx::TokenKind::TOKEN_MINUS:
                op = UnaryExpression::NEGATE;
                break;
            case lx::TokenKind::TOKEN_PLUS:
                op = UnaryExpression::PLUS;
                break;
            case lx::TokenKind::TOKEN_INC:
                op = UnaryExpression::INC;
                break;
            case lx::TokenKind::TOKEN_DEC:
                op = UnaryExpression::DEC;
                break;
            default:
                throw exception::Error("AbstractSyntaxTree::_parse_unary", "Unknown unary operator at ", std::to_string(_previous().line),
                    ":", std::to_string(_previous().column));
                break;
        }

        auto right = _parse_unary();
        return make<UnaryExpression>(op, std::move(right));
    }

    return _parse_call();
}

cplus::ast::ExpressionPtr cplus::ast::AbstractSyntaxTree::_parse_call()
{
    auto expr = _parse_primary();

    for (;;) {
        if (_match({lx::TokenKind::TOKEN_OPEN_PAREN})) {
            expr = _finish_call(std::move(expr));
            continue;
        }
        break;
    }

    return expr;
}

cplus::ast::ExpressionPtr cplus::ast::AbstractSyntaxTree::_finish_call(cplus::ast::ExpressionPtr callee)
{
    auto identifier_expr = dynamic_cast<IdentifierExpression *>(callee.get());

    if (!identifier_expr) {
        throw exception::Error("AbstractSyntaxTree::_finish_call", "Invalid function call");
    }

    const std::string_view function_name = identifier_expr->name;
    auto call_expr = make<CallExpression>(function_name);

    if (!_check(lx::TokenKind::TOKEN_CLOSE_PAREN)) {
        do {
            call_expr->arguments.push_back(_parse_expression());
        } while (_match({lx::TokenKind::TOKEN_COMMA}));
    }

    _consume(lx::TokenKind::TOKEN_CLOSE_PAREN, "Expected ')' after arguments");
    return call_expr;
}

cplus::ast::ExpressionPtr cplus::ast::AbstractSyntaxTree::_parse_primary()
{
    if (_match({lx::TokenKind::TOKEN_INTEGER})) {
        auto &token = _previous();
        const std::string str(token.lexeme);
        const i32 value = std::stoi(str);
        return make<LiteralExpression>(value);
    }

    if (_match({lx::TokenKind::TOKEN_FLOAT})) {
        auto &token = _previous();
        const std::string str(token.lexeme);
        const float value = std::stof(str);
        return make<LiteralExpression>(value);
    }

    if (_match({lx::TokenKind::TOKEN_STRING})) {
        auto &token = _previous();
        return make<LiteralExpression>(token.lexeme);
    }

    if (_match({lx::TokenKind::TOKEN_CHARACTER})) {
        auto &token = _previous();
        return make<LiteralExpression>(token.lexeme);
    }

    if (_match({lx::TokenKind::TOKEN_IDENTIFIER})) {
        auto &token = _previous();

        if (_match({lx::TokenKind::TOKEN_ASSIGN})) {
            auto value = _parse_expression();
            return make<AssignmentExpression>(token.lexeme, std::move(value));
        }

        return make<IdentifierExpression>(token.lexeme);
    }

    if (_match({lx::TokenKind::TOKEN_OPEN_PAREN})) {
        auto expr = _parse_expression();
        _consume(lx::TokenKind::TOKEN_CLOSE_PAREN, "Expected ')' after expression");
        return expr;
    }

    throw exception::Error("AbstractSyntaxTree::_parse_primary", "Unexpected token: ", _peek().lexeme);
}
