#pragma once

#include <memory>
#include <tuple>

namespace cplus {

template<typename... Passes>
class CompilerPipeline
{
    public:
        template<typename... Args>
        CompilerPipeline(Args &&...args) : _passes(std::forward<Args>(args)...)
        {
            /* __ctor__ */
        }

        constexpr ~CompilerPipeline() = default;

        template<typename Input>
        auto execute(const Input &input)
        {
            return execute_impl(input, std::index_sequence_for<Passes...>{});
        }

    private:
        std::tuple<std::unique_ptr<Passes>...> _passes;

        /** @brief single pass */
        template<typename Input>
        auto execute_single_pass(const Input &input, std::index_sequence<0>)
        {
            return std::get<0>(_passes)->run(input);
        }

        /** @brief multiple passes */
        template<typename Input, size_t First, size_t... Rest>
        auto execute_impl_recursive(const Input &input, std::index_sequence<First, Rest...>)
        {
            auto intermediate = std::get<First>(_passes)->run(input);

            if constexpr (sizeof...(Rest) == 0) {
                return std::move(intermediate);
            } else {
                return execute_impl_recursive(std::move(intermediate), std::index_sequence<Rest...>{});// And here
            }
        }

        template<typename Input, size_t... Is>
        auto execute_impl(const Input &input, std::index_sequence<Is...>)
        {
            if constexpr (sizeof...(Is) == 1) {
                return execute_single_pass(input, std::index_sequence<Is...>{});
            } else {
                return execute_impl_recursive(input, std::index_sequence<Is...>{});
            }
        }
};

template<typename... Passes>
auto make_pipeline(std::unique_ptr<Passes>... passes)
{
    return CompilerPipeline<Passes...>(std::move(passes)...);
}

}// namespace cplus
