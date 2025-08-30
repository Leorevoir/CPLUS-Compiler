#include <CPlus/Codegen/x86-64Codegen.hpp>

/**
* public
*/

const std::string cplus::x86_64::Codegen::run(const std::string &ir)
{
    _output.clear();
    return ir;
}

/**
* private
*/

void cplus::x86_64::Codegen::_emit(const std::string &s)
{
    _output += s;
    _output += '\n';
}
