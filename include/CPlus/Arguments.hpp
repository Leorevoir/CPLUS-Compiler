#pragma once

#include <CPlus/Types.hpp>
#include <vector>

namespace cplus {

enum class Flags { FLAG_HELP = 1 << 0, FLAG_VERSION = 1 << 1, FLAG_DEBUG = 1 << 3, FLAG_NONE };

extern i32 cplus_flags;
extern std::vector<cstr> cplus_input_files;
extern cstr cplus_output_file;

void arguments(const i32 argc, const char **argv);

}// namespace cplus
