#pragma once

#include <CPlus/Compiler/Interface.hpp>

namespace cplus {

namespace x86_64 {

class Codegen : public CompilerPass<const std::string, const std::string>
{
    public:
        Codegen() = default;
        ~Codegen() override = default;

        const std::string run(const std::string &ir) override;

    private:
        std::string _output;

        void _emit(const std::string &s);
};

}// namespace x86_64

}// namespace cplus
