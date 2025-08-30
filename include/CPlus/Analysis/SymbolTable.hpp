#pragma once

#include <CPlus/Compiler/Interface.hpp>
#include <CPlus/Parser/Types.hpp>
#include <unordered_map>

namespace cplus {

namespace st {

// clang-format off
struct Symbol {
    enum Kind { VARIABLE, FUNCTION, PARAMETER } kind;
    std::string name;
    ast::TypePtr type;
    std::vector<ast::TypePtr> param_types;
    bool is_const;
    u64 line;
    u64 column;

    constexpr Symbol(Kind k, const std::string&n, ast::TypePtr t, bool const_flag = false, u64 l = 0, u64 c = 0) 
        : kind(k), name(n), type(std::move(t)), is_const(const_flag), line(l), column(c)
    {
        /* __ctor__ */
    }
};

class Scope
{
    public:
        Scope *parent;
        std::unordered_map<std::string, std::unique_ptr<Symbol>> symbols;

        explicit Scope(Scope *p = nullptr) : parent(p)
        {
            symbols.reserve(32);
        }

        inline bool declare(std::string name, std::unique_ptr<Symbol> sym)
        {
            const auto [it, inserted] = symbols.emplace(std::move(name), std::move(sym));

            return inserted;
        }

        inline Symbol *lookup_local(const std::string &name)
        {
            const auto it = symbols.find(name);

            return (it != symbols.end()) ? it->second.get() : nullptr;
        }

        inline Symbol *lookup(const std::string &name)
        {
            Symbol *sym = lookup_local(name);

            if (sym) {
                return sym;
            }
            return parent ? parent->lookup(name) : nullptr;
        }
};
// clang-format on

class SymbolTable : public CompilerPass<std::unique_ptr<ast::Module>, std::unique_ptr<ast::Module>>, public ast::ASTVisitor
{
    public:
        constexpr SymbolTable() = default;
        constexpr ~SymbolTable() override = default;

        std::unique_ptr<ast::Module> run(const std::unique_ptr<ast::Module> &module) override;

    private:
        std::vector<std::unique_ptr<st::Scope>> _scope_stack;
        std::vector<ast::TypePtr> _return_type_stack;
        std::vector<bool> _has_return_stack;

        st::Scope *_current_scope = nullptr;
        cstr _module;

        void visit(ast::LiteralExpression &node) override;
        void visit(ast::IdentifierExpression &node) override;
        void visit(ast::BinaryExpression &node) override;
        void visit(ast::UnaryExpression &node) override;
        void visit(ast::CallExpression &node) override;
        void visit(ast::AssignmentExpression &node) override;
        void visit(ast::ExpressionStatement &node) override;
        void visit(ast::BlockStatement &node) override;
        void visit(ast::VariableDeclaration &node) override;
        void visit(ast::ReturnStatement &node) override;
        void visit(ast::IfStatement &node) override;
        void visit(ast::ForStatement &node) override;
        void visit(ast::ForeachStatement &node) override;
        void visit(ast::CaseStatement &node) override;
        void visit(ast::FunctionDeclaration &node) override;
        void visit(ast::Module &node) override;

        void _add_standard_library();

        void _enter_scope();
        void _exit_scope();

        bool _declare(const std::string &name, st::Symbol::Kind kind, ast::TypePtr type, bool is_const = false, u64 line = 0,
            u64 column = 0);
        st::Symbol *_lookup(const std::string &name);
        ast::TypePtr _infer_type(ast::Expression &expr);
        bool _is_compatible(const ast::Type *left, const ast::Type *right);
};

}// namespace st

}// namespace cplus
