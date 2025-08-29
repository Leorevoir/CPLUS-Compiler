#pragma once

#include <string>

namespace cplus {

// clang-format off
struct FileContent {
    const std::string file;
    const std::string content;
};
// clang-format on

template<typename Input, typename Output>
class CompilerPass
{
    public:
        virtual ~CompilerPass() = default;
        virtual Output run(const Input &input) = 0;
};

}// namespace cplus
