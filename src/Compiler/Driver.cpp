#include <CPlus/Arguments.hpp>
#include <CPlus/Compiler/Driver.hpp>
#include <CPlus/Error.hpp>

#include <fstream>

// clang-format off
cplus::CompilerDriver::CompilerDriver()
    : _pipeline(
        std::make_unique<LexicalAnalyzer>(),
        std::make_unique<AbstractSyntaxTree>()
    )
{
    /* __ctor__ */
}
// clang-format on

void cplus::CompilerDriver::compile(const std::string &source)
{
    const auto ast = _pipeline.execute(source);

    std::ofstream stream(cplus_output_file, std::ios::binary);

    if (!stream.is_open()) {
        throw exception::Error("CompilerDriver::compile", "Failed to open output stream");
    }
}
