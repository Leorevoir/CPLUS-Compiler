#pragma once

#include <CPlus/Analysis/SymbolTable.hpp>
#include <CPlus/Codegen/IntermediateRepresentation.hpp>
#include <CPlus/Compiler/Pipeline.hpp>
#include <CPlus/Parser/AbstractSyntaxTree.hpp>
#include <CPlus/Parser/LexicalAnalyzer.hpp>

namespace cplus {

class CompilerDriver
{
    public:
        CompilerDriver();

        void compile(const FileContent &source);

    private:
        CompilerPipeline<lx::LexicalAnalyzer, ast::AbstractSyntaxTree, st::SymbolTable, ir::IntermediateRepresentation> _pipeline;
};

}// namespace cplus
