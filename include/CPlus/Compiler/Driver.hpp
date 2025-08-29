#pragma once

#include <CPlus/Analysis/SymbolTable.hpp>
#include <CPlus/Compiler/Pipeline.hpp>
#include <CPlus/Parser/AbstractSyntaxTree.hpp>
#include <CPlus/Parser/LexicalAnalyzer.hpp>

namespace cplus {

class CompilerDriver
{
    public:
        CompilerDriver();

        void compile(const std::string &source);

    private:
        CompilerPipeline<LexicalAnalyzer, AbstractSyntaxTree, SymbolTable> _pipeline;
};

}// namespace cplus
