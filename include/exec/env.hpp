#ifndef EXEC_ENV_HPP
#define EXEC_ENV_HPP

#include <exec/queryable.hpp>

#include <concepts>
#include <utility>

namespace exec {
    struct empty_env {};

    struct get_env_t {
        template<queryable QueryableT>
        [[nodiscard]] constexpr decltype(auto) operator()(const QueryableT& queryable) const noexcept {
            if constexpr (requires { queryable.query(*this); }) {
                return queryable.query(*this);
            }
            else {
                return empty_env{};
            }
        }
    };
    inline constexpr get_env_t get_env{};

    template<typename QueryableT>
    using env_of_t = std::invoke_result_t<get_env_t, QueryableT>;
}

#endif // !EXEC_ENV_HPP