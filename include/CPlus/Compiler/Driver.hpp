#pragma once

#include <CPlus/Compiler/Pipeline.hpp>
#include <CPlus/Parser/LexicalAnalyzer.hpp>

namespace cplus {

class CompilerDriver
{
    public:
        CompilerDriver();

        void compile(const std::string &source);

    private:
        CompilerPipeline<LexicalAnalyzer> _pipeline;
};

}// namespace cplus
