#pragma once

#include <CPlus/Types.hpp>

#include <exception>
#include <sstream>
#include <string>
#include <utility>

/**
 * @brief generic Error class
 * @details throw exception::Error("cplus::Core", "failed to open ", <path>, " because: ", dlerror());
 * @return void
 */
namespace cplus::exception {
class Error final : public std::exception
{
    public:
        /**
         * @brief Error constructor template
         * @details takes (what) (where) and (...) it works like va_list
         * @return Error
         */
        template<typename... Args>
        constexpr explicit Error(std::string where, Args &&...args) : _where(std::move(where)), _what(concat(std::forward<Args>(args)...))
        {
            /* empty */
        }

        [[nodiscard]] inline cstr what() const noexcept override
        {
            return _what.c_str();
        }

        [[nodiscard]] inline cstr where() const noexcept
        {
            return _where.c_str();
        }

    private:
        const std::string _where;
        const std::string _what;

        /**
         * @brief Error::concat
         * @details concat strings in va_list ...
         * @return std::string
         */
        template<typename... Args>
        static constexpr inline std::string concat(Args &&...args)
        {
            std::ostringstream oss;

            (oss << ... << args);
            return oss.str();
        }
};
}// namespace cplus::exception
