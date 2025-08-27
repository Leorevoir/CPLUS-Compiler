#pragma once

#include <CPlus/Error.hpp>
#include <iostream>

namespace cplus {

namespace logger {

constexpr const char *CPLUS_GRAY = "\033[38;5;8m";
constexpr const char *CPLUS_RED_BOLD = "\033[1;31m";
constexpr const char *CPLUS_YELLOW = "\033[1;33m";
constexpr const char *CPLUS_GREEN = "\033[1;32m";
constexpr const char *CPLUS_BLUE = "\033[1;34m";
constexpr const char *CPLUS_RESET = "\033[0m";
constexpr const char *CPLUS_MAGENTA = "\033[1;35m";
constexpr const char *CPLUS_ITALIC = "\033[3m";
constexpr const char *CPLUS_BOLD = "\033[1m";

/**
 * @brief logger::error
 * @details takes an exception::Error and display clearly what is the Error, where was it raised and why
 * @param e the exception::Error to display
 * @return void
 */
static inline void error(const cplus::exception::Error &e)
{
    std::cerr << CPLUS_RED_BOLD << "╔════════════════════════════════╗" << CPLUS_RESET << std::endl
              << CPLUS_RED_BOLD << "║       ⚠ ERROR OCCURRED ⚠       ║" << CPLUS_RESET << std::endl
              << CPLUS_RED_BOLD << "╚════════════════════════════════╝" << CPLUS_RESET << std::endl
              << CPLUS_YELLOW << "⮞ Raised by: " << CPLUS_RESET << e.where() << std::endl
              << CPLUS_YELLOW << "⮞ Reason:    " << CPLUS_RESET << e.what() << std::endl;
}

template<typename... Args>
static constexpr inline void debug([[maybe_unused]] Args &&...args)
{
#ifdef CPLUS_DEBUG
    std::ostringstream oss;
    const i32 __attribute__((unused)) _[] = {0, (oss << args, 0)...};
    std::cout << CPLUS_MAGENTA << "[DEBUG] " << CPLUS_RESET << CPLUS_ITALIC << oss.str() << CPLUS_RESET << std::endl;
#endif
}

template<typename... Args>
static constexpr inline void info(Args &&...args)
{
    std::ostringstream oss;
    const i32 __attribute__((unused)) _[] = {0, (oss << args, 0)...};
    std::cout << CPLUS_YELLOW << "[INFO] " << CPLUS_RESET << oss.str() << std::endl;
}

}// namespace logger

}// namespace cplus
