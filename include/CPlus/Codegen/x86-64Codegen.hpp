#pragma once

#include <CPlus/Compiler/Interface.hpp>
#include <CPlus/Types.hpp>

#include <unordered_map>

namespace cplus {

namespace x86_64 {

class Codegen : public CompilerPass<const std::string, const std::string>
{
    public:
        Codegen() = default;
        ~Codegen() override = default;

        const std::string run(const std::string &ir) override;

    private:
        u64 _stack_offset = 0;
        i32 _next_stack_offset = -4;
        u8 _register_index = 0;

        std::unordered_map<std::string, std::string> _var_locations;

        std::string _current_function;
        std::string _output;
        std::string _ir;

        void _emit(const std::string &s);

        void _prologue();
        void _generate();
        void _epilogue();

        void _generate_line(const std::string &line);

        void _emit_function_declaration(const std::string &line);
        void _emit_function_start();
        void _emit_function_end();

        void _emit_call_instruction(const std::string &line);
        void _emit_label(const std::string &line);
        void _emit_function_call(const std::string &line);
        void _emit_assignement(const std::string &line);
        void _emit_mov(const std::string &dest, const std::string &src);
        void _emit_branch(const std::string &label);
        void _emit_return(const std::string &value);
        void _emit_phi(const std::string &dest, const std::string &rhs);
        void _emit_call_result(const std::string &dest);
        void _emit_arg_load(const std::string &dest, const std::string &rhs);
        void _emit_binary_op(const std::string &dest, const std::string &src1, const std::string &op);
        void _emit_unary_op(const std::string &dest, const std::string &src1, const std::string &op);
        void _emit_div(const std::string &dest, const std::string &src1, const bool is_mod = false);
        void _emit_compare(const std::string &src1, const std::string &src2);

        const std::string _get_stack_location(const std::string &var);
        const std::string _get_operand(const std::string &operand);
};

}// namespace x86_64

}// namespace cplus
