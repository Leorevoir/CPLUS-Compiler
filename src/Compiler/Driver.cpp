#include <CPlus/Arguments.hpp>
#include <CPlus/Compiler/Driver.hpp>
#include <CPlus/Error.hpp>

#include <fstream>

// clang-format off
cplus::CompilerDriver::CompilerDriver()
    : _pipeline(
        std::make_unique<LexicalAnalyzer>()
    )
{
    /* __ctor__ */
}
// clang-format on

void cplus::CompilerDriver::compile(const std::string &source)
{
    const auto tokens = _pipeline.execute(source);

    std::ofstream stream(cplus_output_file, std::ios::binary);

    if (!stream.is_open()) {
        throw exception::Error("CompilerDriver::compile", "Failed to open output stream");
    }

    for (const auto &token : tokens) {
        stream << "Token: " << static_cast<int>(token.kind) << "\n";
    }
}
