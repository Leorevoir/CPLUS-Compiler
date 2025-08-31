#pragma once

#include <CPlus/Compiler/Interface.hpp>
#include <CPlus/Types.hpp>

namespace cplus {

namespace x86_64 {

class Codegen : public CompilerPass<const std::string, const std::string>
{
    public:
        Codegen() = default;
        ~Codegen() override = default;

        const std::string run(const std::string &ir) override;

    private:
        i32 _stack_offset = 0;
        std::string _current_function;
        std::string _output;
        std::string _ir;

        void _emit(const std::string &s);

        void _prologue();
        void _generate();

        void _generate_line(const std::string &line);

        void _emit_function_declaration(const std::string &line);
        void _emit_function_start();
        void _emit_function_end();
};

}// namespace x86_64

}// namespace cplus
