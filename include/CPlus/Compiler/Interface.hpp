#pragma once

namespace cplus {

template<typename Input, typename Output>
class CompilerPass
{
    public:
        virtual ~CompilerPass() = default;
        virtual Output run(const Input &input) = 0;
};

}// namespace cplus
