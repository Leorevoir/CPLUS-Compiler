#include <CPlus/Arguments.hpp>
#include <CPlus/Compiler/Driver.hpp>
#include <CPlus/Error.hpp>

#include <fstream>

// clang-format off
cplus::CompilerDriver::CompilerDriver()
    : _pipeline(
        std::make_unique<lx::LexicalAnalyzer>(),
        std::make_unique<ast::AbstractSyntaxTree>(),
        std::make_unique<st::SymbolTable>(),
        std::make_unique<ir::IntermediateRepresentation>()
    )
{
    /* __ctor__ */
}
// clang-format on

void cplus::CompilerDriver::compile(const FileContent &source)
{
    const auto ir = _pipeline.execute(source);

    std::ofstream stream(cplus_output_file, std::ios::binary);

    if (!stream.is_open()) {
        throw exception::Error("CompilerDriver::compile", "Failed to open output stream");
    }

    stream.write(ir.data(), static_cast<std::streamsize>(ir.size()));
    stream.close();
}
