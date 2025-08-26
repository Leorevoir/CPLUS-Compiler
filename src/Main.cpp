#include "CPlus/Arguments.hpp"
#include "CPlus/Logger.hpp"
#include "CPlus/Macros.hpp"

int main(const int argc, const char **argv)
{
    try {
        cplus::arguments(argc, argv);

    } catch (const cplus::exception::Error &e) {
        cplus::logger::error(e);
        return CPLUS_ERROR;
    }
    return CPLUS_SUCCESS;
}
