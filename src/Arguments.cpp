#include <CPlus/Arguments.hpp>
#include <CPlus/Error.hpp>
#include <CPlus/Macros.hpp>
#include <CPlus/Types.hpp>

#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>

#include <sys/stat.h>

int cplus::cplus_flags = 0;
std::vector<cplus::cstr> cplus::cplus_input_files;
cplus::cstr cplus::cplus_output_file = "out.bin";

static inline void usage()
{
    std::cout << "Usage: cplus [options] <input.cp>" << std::endl
              << "Options:" << std::endl
              << "  -v,     --version   Show version information" << std::endl
              << "  -help,  --help      Show this help message" << std::endl
              << "  -d,     --debug     Enable debug mode" << std::endl
              << "  -o,     --output    Output file" << std::endl;
}

static inline void version()
{
    std::cout << "CPlus Version " << CPLUS_VERSION << std::endl;
}

static inline void output(cplus::cstr filename)
{
    static bool output_set = false;

    if (output_set) {
        throw cplus::exception::Error("cplus::Arguments", "Output file already set to ", cplus::cplus_output_file);
    }

    cplus::cplus_output_file = filename;
    output_set = true;
}

static inline void input(cplus::cstr filename)
{
    struct stat st;

    if (stat(filename, &st) != 0) {
        throw cplus::exception::Error("cplus::Arguments", "Input file does not exist: ", filename);
    }
    if (!S_ISREG(st.st_mode)) {
        throw cplus::exception::Error("cplus::Arguments", "Input file is not a regular file: ", filename);
    }
    cplus::cplus_input_files.push_back(filename);
}

// clang-format off
static const inline std::unordered_map<std::string, std::function<void()>> _flags = {
    {"-help",
        []() {
            usage();
            std::exit(0);
        }},
    {"--help",
        []() {
            usage();
            std::exit(0);
        }},
    {"-v",
        []() {
            version();
            std::exit(0);
        }},
    {"--version",
        []() {
            version();
            std::exit(0);
        }},
    {"-d",
        []() {
            cplus::cplus_flags |= static_cast<int>(cplus::Flags::FLAG_DEBUG);
        }},
    {"--debug",
        []() {
            cplus::cplus_flags |= static_cast<int>(cplus::Flags::FLAG_DEBUG);
        }},
};
// clang-format on

void cplus::arguments(const i32 argc, const char **argv)
{
    for (i32 i = 0; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg.starts_with('-')) {
            const auto it = _flags.find(arg);

            if (it != _flags.end()) {
                it->second();

            } else if (arg == "-o" || arg == "--output") {

                if (i + 1 >= argc) {
                    throw cplus::exception::Error("cplus::Arguments", "Missing output file after ", arg);
                }

                output(argv[++i]);

            } else {
                throw cplus::exception::Error("cplus::Arguments", "Unknown argument: ", arg);
            }

        } else {
            input(argv[i]);
        }
    }

    if (cplus_input_files.empty()) {
        throw cplus::exception::Error("cplus::Arguments", "No input files provided");
    }
}
