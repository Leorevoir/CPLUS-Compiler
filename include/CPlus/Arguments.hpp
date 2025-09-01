#pragma once

#include <CPlus/Types.hpp>
#include <vector>

namespace cplus {

enum Flags {
    FLAG_HELP = 1 << 0,
    FLAG_VERSION = 1 << 1,
    FLAG_SHOW_AST = 1 << 3,
    FLAG_SHOW_TOKENS = 1 << 4,
    FLAG_SHOW_IR = 1 << 5,
    FLAG_NONE,
};

extern i32 cplus_flags;
extern std::vector<cstr> cplus_input_files;
extern cstr cplus_output_file;

void arguments(const i32 argc, const char **argv);

}// namespace cplus
