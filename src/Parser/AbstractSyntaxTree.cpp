#include <CPlus/Arguments.hpp>
#include <CPlus/Parser/AbstractSyntaxTree.hpp>
#include <CPlus/Parser/Logger.hpp>

/**
* public
*/

std::unique_ptr<cplus::ast::Program> cplus::AbstractSyntaxTree::run(const std::vector<Token> &tokens)
{
    _tokens = tokens;
    _current = 0;

    return _parse_program();
}

/**
 * helpers
 */

bool cplus::AbstractSyntaxTree::_is_at_end() const
{
    if (_current >= _tokens.size()) {
        return true;
    }
    return _tokens[_current].kind == TokenKind::TOKEN_EOF;
}

const cplus::Token &cplus::AbstractSyntaxTree::_peek() const
{
    if (_current >= _tokens.size()) {
        static const Token eof_token{TokenKind::TOKEN_EOF, "", 0, 0};
        return eof_token;
    }
    return _tokens[_current];
}

const cplus::Token &cplus::AbstractSyntaxTree::_previous() const
{
    if (_current == 0) {
        static const Token start_token{TokenKind::TOKEN_EOF, "", 0, 0};
        return start_token;
    }
    return _tokens[_current - 1];
}

const cplus::Token &cplus::AbstractSyntaxTree::_advance()
{
    if (!_is_at_end()) {
        ++_current;
    }
    return _previous();
}

const cplus::Token &cplus::AbstractSyntaxTree::_consume(TokenKind kind, const std::string &message)
{
    if (_check(kind)) {
        return _advance();
    }

    auto &current = _peek();
    throw exception::Error("AbstractSyntaxTree::_consume", message, " at ", std::to_string(current.line), ":",
        std::to_string(current.column));
}

bool cplus::AbstractSyntaxTree::_check(TokenKind kind) const
{
    if (_current >= _tokens.size()) {
        return false;
    }
    return _tokens[_current].kind == kind;
}

bool cplus::AbstractSyntaxTree::_match(const std::initializer_list<TokenKind> &kinds)
{
    for (auto &kind : kinds) {
        if (_check(kind)) {
            _advance();
            return true;
        }
    }
    return false;
}

void cplus::AbstractSyntaxTree::_synchronize()
{
    _advance();

    while (!_is_at_end()) {

        if (_previous().kind == TokenKind::TOKEN_SEMICOLON) {
            return;
        }

        switch (_peek().kind) {
            case TokenKind::TOKEN_DEF:
            case TokenKind::TOKEN_CONST:
            case TokenKind::TOKEN_RETURN:
            case TokenKind::TOKEN_STRUCT:
            case TokenKind::TOKEN_IF:
            case TokenKind::TOKEN_FOR:
            case TokenKind::TOKEN_FOREACH:
            case TokenKind::TOKEN_WHILE:
                return;
            default:
                break;
        }
        _advance();
    }
}

/**
* private
*/

std::unique_ptr<cplus::ast::Program> cplus::AbstractSyntaxTree::_parse_program()
{
    auto program = ast::make<ast::Program>();

    while (!_is_at_end()) {
        if (auto decl = _parse_declaration()) {
            program->declarations.push_back(std::move(decl));
        }
    }

    if (cplus_flags & FLAG_DEBUG) {
        ast::ASTLogger logger;
        logger.show(*program);
    }

    return program;
}

cplus::ast::StatementPtr cplus::AbstractSyntaxTree::_parse_declaration()
{
    try {

        if (_match({TokenKind::TOKEN_DEF})) {
            return _parse_function_declaration();
        }
        if (_match({TokenKind::TOKEN_CONST})) {
            return _parse_variable_declaration(true);
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
cplus::ast::StatementPtr cplus::AbstractSyntaxTree::_parse_function_declaration()
{
    auto name = _consume(TokenKind::TOKEN_IDENTIFIER, "Expected function name");
    auto func = ast::make<ast::FunctionDeclaration>(name.lexeme);

    func->line = name.line;
    func->column = name.column;

    _consume(TokenKind::TOKEN_OPEN_PAREN, "Expected '(' after function name");

    if (!_check(TokenKind::TOKEN_CLOSE_PAREN)) {

        do {
            auto param_name = _consume(TokenKind::TOKEN_IDENTIFIER, "Expected parameter name");

            ast::TypePtr param_type = nullptr;
            if (_match({TokenKind::TOKEN_COLON})) {
                param_type = _parse_type();
            }

            func->parameters.emplace_back(param_name.lexeme, std::move(param_type));
        } while (_match({TokenKind::TOKEN_COMMA}));
    }

    _consume(TokenKind::TOKEN_CLOSE_PAREN, "Expected ')' after parameters");

    if (_match({TokenKind::TOKEN_ARROW})) {
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
cplus::ast::StatementPtr cplus::AbstractSyntaxTree::_parse_variable_declaration(bool is_const)
{
    auto name = _consume(TokenKind::TOKEN_IDENTIFIER, "Expected variable name");
    auto var_decl = ast::make<ast::VariableDeclaration>(name.lexeme, is_const);

    var_decl->line = name.line;
    var_decl->column = name.column;

    if (_match({TokenKind::TOKEN_COLON})) {
        var_decl->declared_type = _parse_type();
    }
    if (_match({TokenKind::TOKEN_ASSIGN})) {
        var_decl->initializer = _parse_expression();
    }

    _consume(TokenKind::TOKEN_SEMICOLON, "Expected ';' after variable declaration");
    return var_decl;
}

cplus::ast::TypePtr cplus::AbstractSyntaxTree::_parse_type()
{
    auto type_token = _consume(TokenKind::TOKEN_IDENTIFIER, "Expected type name");
    ast::Type::Kind kind = ast::from_string(type_token.lexeme);

    return ast::make<ast::Type>(kind, type_token.lexeme);
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
cplus::ast::StatementPtr cplus::AbstractSyntaxTree::_parse_statement()
{
    if (_match({TokenKind::TOKEN_IF})) {
        return _parse_if_statement();
    }
    if (_match({TokenKind::TOKEN_FOR})) {
        return _parse_for_statement();
    }
    if (_match({TokenKind::TOKEN_FOREACH})) {
        return _parse_foreach_statement();
    }
    if (_match({TokenKind::TOKEN_CASE})) {
        return _parse_case_statement();
    }
    if (_match({TokenKind::TOKEN_RETURN})) {
        return _parse_return_statement();
    }
    if (_check(TokenKind::TOKEN_OPEN_BRACE)) {
        return _parse_block_statement();
    }

    if (_check(TokenKind::TOKEN_IDENTIFIER)) {
        const u64 pos = _current;
        _advance();

        if (_check(TokenKind::TOKEN_COLON) || _check(TokenKind::TOKEN_ASSIGN)) {
            _current = pos;
            return _parse_variable_declaration(false);
        }

        _current = pos;
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
cplus::ast::StatementPtr cplus::AbstractSyntaxTree::_parse_block_statement()
{
    _consume(TokenKind::TOKEN_OPEN_BRACE, "Expected '{'");

    auto block = ast::make<ast::BlockStatement>();

    while (!_check(TokenKind::TOKEN_CLOSE_BRACE) && !_is_at_end()) {

        if (auto stmt = _parse_declaration()) {
            block->statements.push_back(std::move(stmt));
        }
    }

    _consume(TokenKind::TOKEN_CLOSE_BRACE, "Expected '}'");
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
cplus::ast::StatementPtr cplus::AbstractSyntaxTree::_parse_if_statement()
{
    _consume(TokenKind::TOKEN_OPEN_PAREN, "Expected '(' after 'if'");
    auto condition = _parse_expression();
    _consume(TokenKind::TOKEN_CLOSE_PAREN, "Expected ')' after if condition");

    auto then_stmt = _parse_statement();
    auto if_stmt = ast::make<ast::IfStatement>(std::move(condition), std::move(then_stmt));

    if (_match({TokenKind::TOKEN_ELSE})) {
        if_stmt->else_statement = _parse_statement();
    }

    return if_stmt;
}

/**
* @brief for loop statement
* @syntax for (initializer; condition; increment) { body }
*
* for (i:int = 0; i < 10; ++i) {
*     print(i);
* }
*/
cplus::ast::StatementPtr cplus::AbstractSyntaxTree::_parse_for_statement()
{
    _consume(TokenKind::TOKEN_OPEN_PAREN, "Expected '(' after 'for'");

    auto for_stmt = ast::make<ast::ForStatement>();

    if (_match({TokenKind::TOKEN_SEMICOLON})) {
        for_stmt->initializer = nullptr;
    } else {
        for_stmt->initializer = _parse_declaration();
    }

    if (!_check(TokenKind::TOKEN_SEMICOLON)) {
        for_stmt->condition = _parse_expression();
    }
    _consume(TokenKind::TOKEN_SEMICOLON, "Expected ';' after for loop condition");

    if (!_check(TokenKind::TOKEN_CLOSE_PAREN)) {
        for_stmt->increment = _parse_expression();
    }
    _consume(TokenKind::TOKEN_CLOSE_PAREN, "Expected ')' after for clauses");

    for_stmt->body = _parse_statement();

    return for_stmt;
}

/**
* @brief foreach loop statement
* @syntax foreach (iterator in iterable) { body }
*
* foreach (item in collection) {
*     print(item);
* }
*
* better than for (i = 0; i < collection.size(); ++i) {print(collection[i]);}
*/
cplus::ast::StatementPtr cplus::AbstractSyntaxTree::_parse_foreach_statement()
{
    _consume(TokenKind::TOKEN_OPEN_PAREN, "Expected '(' after 'foreach'");

    auto iterator_token = _consume(TokenKind::TOKEN_IDENTIFIER, "Expected iterator variable name");
    _consume(TokenKind::TOKEN_IN, "Expected 'in' keyword");

    auto iterable = _parse_expression();
    _consume(TokenKind::TOKEN_CLOSE_PAREN, "Expected ')' after foreach");

    auto body = _parse_statement();

    return ast::make<ast::ForeachStatement>(iterator_token.lexeme, std::move(iterable), std::move(body));
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
cplus::ast::StatementPtr cplus::AbstractSyntaxTree::_parse_case_statement()
{
    _consume(TokenKind::TOKEN_OPEN_PAREN, "Expected '(' after 'case'");
    auto expression = _parse_expression();
    _consume(TokenKind::TOKEN_CLOSE_PAREN, "Expected ')' after case expression");

    auto case_stmt = ast::make<ast::CaseStatement>(std::move(expression));

    _consume(TokenKind::TOKEN_OPEN_BRACE, "Expected '{' before case clauses");

    while (!_check(TokenKind::TOKEN_CLOSE_BRACE) && !_is_at_end()) {
        ast::CaseStatement::CaseClause clause;

        if (_match({TokenKind::TOKEN_DEFAULT})) {
            clause.value = nullptr;//<< default case
        } else {
            clause.value = _parse_expression();
        }

        _consume(TokenKind::TOKEN_COLON, "Expected ':' after case value");

        // Parse statements until next case or end
        while (!_check(TokenKind::TOKEN_CLOSE_BRACE) && !_is_at_end() && !_check(TokenKind::TOKEN_DEFAULT)
            && !(_check(TokenKind::TOKEN_INTEGER) || _check(TokenKind::TOKEN_IDENTIFIER))) {
            if (auto stmt = _parse_statement()) {
                clause.statements.push_back(std::move(stmt));
            }
        }

        case_stmt->clauses.push_back(std::move(clause));
    }

    _consume(TokenKind::TOKEN_CLOSE_BRACE, "Expected '}' after case clauses");

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
cplus::ast::StatementPtr cplus::AbstractSyntaxTree::_parse_return_statement()
{
    cplus::ast::ExpressionPtr value = nullptr;

    if (!_check(TokenKind::TOKEN_SEMICOLON)) {
        value = _parse_expression();
    }

    _consume(TokenKind::TOKEN_SEMICOLON, "Expected ';' after return value");

    return ast::make<ast::ReturnStatement>(std::move(value));
}

cplus::ast::StatementPtr cplus::AbstractSyntaxTree::_parse_expression_statement()
{
    auto expr = _parse_expression();
    _consume(TokenKind::TOKEN_SEMICOLON, "Expected ';' after expression");

    return ast::make<ast::ExpressionStatement>(std::move(expr));
}

cplus::ast::ExpressionPtr cplus::AbstractSyntaxTree::_parse_expression()
{
    return _parse_logical_or();
}

/**
* @brief parse logical OR
* @syntax expression || expression
*
* if (a > 0 || b < 0) { ... }
*/
cplus::ast::ExpressionPtr cplus::AbstractSyntaxTree::_parse_logical_or()
{
    auto expr = _parse_logical_and();

    while (_match({TokenKind::TOKEN_CMP_OR})) {
        auto op = ast::BinaryExpression::OR;
        auto right = _parse_logical_and();
        expr = ast::make<ast::BinaryExpression>(std::move(expr), op, std::move(right));
    }

    return expr;
}

/**
* @brief parse logical AND
* @syntax expression && expression
*
* if (a > 0 && b < 0) { ... }
*/
cplus::ast::ExpressionPtr cplus::AbstractSyntaxTree::_parse_logical_and()
{
    auto expr = _parse_equality();

    while (_match({TokenKind::TOKEN_CMP_AND})) {
        auto op = ast::BinaryExpression::AND;
        auto right = _parse_equality();
        expr = ast::make<ast::BinaryExpression>(std::move(expr), op, std::move(right));
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
cplus::ast::ExpressionPtr cplus::AbstractSyntaxTree::_parse_equality()
{
    auto expr = _parse_comparison();

    while (_match({TokenKind::TOKEN_EQ, TokenKind::TOKEN_NEQ})) {
        auto op = (_previous().kind == TokenKind::TOKEN_EQ) ? ast::BinaryExpression::EQ : ast::BinaryExpression::NEQ;
        auto right = _parse_comparison();
        expr = ast::make<ast::BinaryExpression>(std::move(expr), op, std::move(right));
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
cplus::ast::ExpressionPtr cplus::AbstractSyntaxTree::_parse_comparison()
{
    auto expr = _parse_term();

    while (_match({TokenKind::TOKEN_GT, TokenKind::TOKEN_GTE, TokenKind::TOKEN_LT, TokenKind::TOKEN_LTE})) {
        ast::BinaryExpression::Operator op;
        switch (_previous().kind) {
            case TokenKind::TOKEN_GT:
                op = ast::BinaryExpression::GT;
                break;
            case TokenKind::TOKEN_GTE:
                op = ast::BinaryExpression::GTE;
                break;
            case TokenKind::TOKEN_LT:
                op = ast::BinaryExpression::LT;
                break;
            case TokenKind::TOKEN_LTE:
                op = ast::BinaryExpression::LTE;
                break;
            default:
                throw exception::Error("AbstractSyntaxTree::_parse_comparison", "Unknown comparison operator at ",
                    std::to_string(_previous().line), ":", std::to_string(_previous().column));
                break;
        }

        auto right = _parse_term();
        expr = ast::make<ast::BinaryExpression>(std::move(expr), op, std::move(right));
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
cplus::ast::ExpressionPtr cplus::AbstractSyntaxTree::_parse_term()
{
    auto expr = _parse_factor();

    while (_match({TokenKind::TOKEN_MINUS, TokenKind::TOKEN_PLUS})) {
        auto op = (_previous().kind == TokenKind::TOKEN_MINUS) ? ast::BinaryExpression::SUB : ast::BinaryExpression::ADD;
        auto right = _parse_factor();
        expr = ast::make<ast::BinaryExpression>(std::move(expr), op, std::move(right));
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
cplus::ast::ExpressionPtr cplus::AbstractSyntaxTree::_parse_factor()
{
    auto expr = _parse_unary();

    while (_match({TokenKind::TOKEN_SLASH, TokenKind::TOKEN_ASTERISK, TokenKind::TOKEN_MODULO})) {
        ast::BinaryExpression::Operator op;
        switch (_previous().kind) {
            case TokenKind::TOKEN_SLASH:
                op = ast::BinaryExpression::DIV;
                break;
            case TokenKind::TOKEN_ASTERISK:
                op = ast::BinaryExpression::MUL;
                break;
            case TokenKind::TOKEN_MODULO:
                op = ast::BinaryExpression::MOD;
                break;
            default:
                throw exception::Error("AbstractSyntaxTree::_parse_factor", "Unknown factor operator at ", std::to_string(_previous().line),
                    ":", std::to_string(_previous().column));
                break;
        }

        auto right = _parse_unary();
        expr = ast::make<ast::BinaryExpression>(std::move(expr), op, std::move(right));
    }

    return expr;
}

/**
* @brief parse unary
* @syntax -expression
* @syntax !expression
* @syntax +expression
*/
cplus::ast::ExpressionPtr cplus::AbstractSyntaxTree::_parse_unary()
{
    if (_match({TokenKind::TOKEN_CMP_NOT, TokenKind::TOKEN_MINUS, TokenKind::TOKEN_PLUS})) {
        ast::UnaryExpression::Operator op;
        switch (_previous().kind) {
            case TokenKind::TOKEN_CMP_NOT:
                op = ast::UnaryExpression::NOT;
                break;
            case TokenKind::TOKEN_MINUS:
                op = ast::UnaryExpression::NEGATE;
                break;
            case TokenKind::TOKEN_PLUS:
                op = ast::UnaryExpression::PLUS;
                break;
            default:
                throw exception::Error("AbstractSyntaxTree::_parse_unary", "Unknown unary operator at ", std::to_string(_previous().line),
                    ":", std::to_string(_previous().column));
                break;
        }

        auto right = _parse_unary();
        return ast::make<ast::UnaryExpression>(op, std::move(right));
    }

    return _parse_call();
}

cplus::ast::ExpressionPtr cplus::AbstractSyntaxTree::_parse_call()
{
    auto expr = _parse_primary();

    for (;;) {
        if (_match({TokenKind::TOKEN_OPEN_PAREN})) {
            expr = _finish_call(std::move(expr));
            continue;
        }
        break;
    }

    return expr;
}

cplus::ast::ExpressionPtr cplus::AbstractSyntaxTree::_finish_call(cplus::ast::ExpressionPtr callee)
{
    auto identifier_expr = dynamic_cast<ast::IdentifierExpression *>(callee.get());

    if (!identifier_expr) {
        throw cplus::exception::Error("AbstractSyntaxTree::_finish_call", "Invalid function call");
    }

    const std::string_view function_name = identifier_expr->name;
    auto call_expr = ast::make<ast::CallExpression>(function_name);

    if (!_check(TokenKind::TOKEN_CLOSE_PAREN)) {
        do {
            call_expr->arguments.push_back(_parse_expression());
        } while (_match({TokenKind::TOKEN_COMMA}));
    }

    _consume(TokenKind::TOKEN_CLOSE_PAREN, "Expected ')' after arguments");
    return call_expr;
}

cplus::ast::ExpressionPtr cplus::AbstractSyntaxTree::_parse_primary()
{
    if (_match({TokenKind::TOKEN_INTEGER})) {
        auto &token = _previous();
        const std::string str(token.lexeme);
        i64 value = std::stoll(str);
        return ast::make<ast::LiteralExpression>(value);
    }

    if (_match({TokenKind::TOKEN_FLOAT})) {
        auto &token = _previous();
        const std::string str(token.lexeme);
        double value = std::stod(str);
        return ast::make<ast::LiteralExpression>(value);
    }

    if (_match({TokenKind::TOKEN_STRING})) {
        auto &token = _previous();
        return ast::make<ast::LiteralExpression>(token.lexeme);
    }

    if (_match({TokenKind::TOKEN_CHARACTER})) {
        auto &token = _previous();
        return ast::make<ast::LiteralExpression>(token.lexeme);
    }

    if (_match({TokenKind::TOKEN_IDENTIFIER})) {
        auto &token = _previous();

        if (_match({TokenKind::TOKEN_ASSIGN})) {
            auto value = _parse_expression();
            return ast::make<ast::AssignmentExpression>(token.lexeme, std::move(value));
        }

        return ast::make<ast::IdentifierExpression>(token.lexeme);
    }

    if (_match({TokenKind::TOKEN_OPEN_PAREN})) {
        auto expr = _parse_expression();
        _consume(TokenKind::TOKEN_CLOSE_PAREN, "Expected ')' after expression");
        return expr;
    }

    throw cplus::exception::Error("AbstractSyntaxTree::_parse_primary", "Unexpected token: ", _peek().lexeme);
}
