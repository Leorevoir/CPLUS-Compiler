#include <CPlus/Arguments.hpp>
#include <CPlus/Error.hpp>
#include <CPlus/Logger.hpp>
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

static constexpr auto bold = cplus::logger::CPLUS_BOLD;
static constexpr auto reset = cplus::logger::CPLUS_RESET;
static constexpr auto green = cplus::logger::CPLUS_GREEN;
static constexpr auto yellow = cplus::logger::CPLUS_YELLOW;
static constexpr auto blue = cplus::logger::CPLUS_BLUE;
static constexpr auto gray = cplus::logger::CPLUS_GRAY;
static constexpr auto red_bold = cplus::logger::CPLUS_RED_BOLD;

constexpr auto print_option = [](const std::string &flags, const std::string &description) -> void {
    std::cout << "  " << yellow << flags << reset << gray << "   " << description << reset << std::endl;
};

static constexpr inline void usage()
{
    std::cout << bold << "USAGE: " << reset << green << "cplus " << reset << yellow << "[options] " << reset << blue << "<input.cp>"
              << reset << std::endl
              << std::endl
              << bold << "OPTIONS:" << reset << std::endl;

    print_option("-v,  --version", "Show version information");
    print_option("-help, --help", " Show this help message");
    print_option("-d,  --debug", "  Enable debug mode");
    print_option("-o,  --output", " Output file");
    print_option("-t,  --show-tokens", " Show Tokens");
    print_option("-a,  --show-ast", " Show AST");

    std::cout << std::endl;
    std::exit(CPLUS_SUCCESS);
}

static constexpr inline void version()
{
    std::cout << bold << "CPlus " << reset << "v." << CPLUS_VERSION << std::endl
              << "Not C, not C++, just " << red_bold << "C+" << reset << std::endl
              << yellow << "Copyright (c) 2025-2026 CPlus Contributors" << reset << std::endl;
    std::exit(CPLUS_SUCCESS);
}

static constexpr inline void output(cplus::cstr filename)
{
    static bool output_set = false;

    if (output_set) {
        throw cplus::exception::Error("cplus::Arguments", "Output file already set to ", cplus::cplus_output_file);
    }

    cplus::cplus_output_file = filename;
    output_set = true;
}

static constexpr inline void input(cplus::cstr filename)
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
    {"-h",
        []() {
            usage();
        }},
    {"-help",
        []() {
            usage();
        }},
    {"--help",
        []() {
            usage();
        }},
    {"-v",
        []() {
            version();
        }},
    {"--version",
        []() {
            version();
        }},
    {"-d",
        []() {
            cplus::cplus_flags |= (cplus::Flags::FLAG_DEBUG);
        }},
    {"--debug",
        []() {
            cplus::cplus_flags |= (cplus::Flags::FLAG_DEBUG);
        }},
    {"-t",
        []() {
            cplus::cplus_flags |= (cplus::Flags::FLAG_SHOW_TOKENS);
        }},
    {"--show-tokens",
        []() {
            cplus::cplus_flags |= (cplus::Flags::FLAG_SHOW_TOKENS);
        }},
    {"-a",
        []() {
            cplus::cplus_flags |= (cplus::Flags::FLAG_SHOW_AST);
        }},
    {"--show-ast",
        []() {
            cplus::cplus_flags |= (cplus::Flags::FLAG_SHOW_AST);
        }},
};
// clang-format on

void cplus::arguments(const i32 argc, const char **argv)
{
    for (i32 i = 1; i < argc; ++i) {
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
