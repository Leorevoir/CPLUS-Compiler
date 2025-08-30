#include <CPlus/Arguments.hpp>
#include <CPlus/Compiler/Driver.hpp>
#include <CPlus/Logger.hpp>
#include <CPlus/Macros.hpp>

#include <fstream>

static std::string read_file_content(const std::string &filename)
{
    std::ifstream file(filename, std::ios::in | std::ios::binary);

    if (!file) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    file.seekg(0, std::ios::end);
    const auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string content;
    content.resize(static_cast<cplus::u64>(size));

    file.read(&content[0], size);
    return content;
}

//TODO: in the future multithread this routine
static void cplus_compiler_routine()
{
    for (const auto &file : cplus::cplus_input_files) {
        cplus::CompilerDriver driver;
        const std::string content = read_file_content(file);

        if (cplus::cplus_flags & cplus::FLAG_DEBUG) {
            cplus::logger::info("Compiling file: ", file);
        }

        driver.compile({file, content});
    }
}

int main(const int argc, const char **argv)
{
    try {
        cplus::arguments(argc, argv);

        cplus_compiler_routine();

    } catch (const cplus::exception::Error &e) {
        cplus::logger::error(e);
        return CPLUS_ERROR;
    }
    return CPLUS_SUCCESS;
}
