#include <CPlus/Arguments.hpp>
#include <CPlus/Compiler/Driver.hpp>
#include <CPlus/Error.hpp>
#include <cstdlib>
#include <fstream>

// clang-format off
cplus::CompilerDriver::CompilerDriver()
    : _pipeline(
        std::make_unique<lx::LexicalAnalyzer>(),
        std::make_unique<ast::AbstractSyntaxTree>(),
        std::make_unique<st::SymbolTable>(),
        std::make_unique<ir::IntermediateRepresentation>(),
        std::make_unique<x86_64::Codegen>()
    )
{
    /* __ctor__ */
}
// clang-format on

static bool _call(const std::string &what, const std::string &args)
{
    const std::string cmd = what + " " + args;
    const cplus::i32 ret = std::system(cmd.c_str());

    if (ret == -1) {
        return false;
    }
    return true;
}

void cplus::CompilerDriver::compile(const FileContent &source)
{
    const auto &x86_64 = _pipeline.execute(source);
    const std::string filename = source.file + ".s";
    const std::string object = source.file + ".o";
    std::ofstream stream(filename, std::ios::binary);

    if (!stream.is_open()) {
        throw exception::Error("CompilerDriver::compile", "Failed to open output stream");
    }

    stream.write(x86_64.data(), static_cast<std::streamsize>(x86_64.size()));
    stream.close();
    logger::info("Assembly code generated to ", filename);

    if (!_call("as", filename + " -o " + object)) {
        throw exception::Error("CompilerDriver::compile", "Failed to call 'as'");
    }
    logger::info("Object file generated to ", object);

    if (!_call("ld", object + " -o " + cplus_output_file)) {
        throw exception::Error("CompilerDriver::compile", "Failed to call 'ld'");
    }
    logger::info("Executable linked to ", cplus_output_file);
}
