#ifndef EXEC_STOP_TOKEN_HPP
#define EXEC_STOP_TOKEN_HPP

#include "exec/query.hpp"

#include <stop_token>
#include <type_traits>
#include <utility>

namespace exec {
    struct get_stop_token_t {
        template<typename EnvT>
        requires requires{ std::declval<EnvT>().template query<get_stop_token_t>({}); }
        [[nodiscard]] constexpr decltype(auto) operator()(auto&& env) const noexcept {
            return std::forward<decltype(env)>(env).query(*this);
        }

        [[nodiscard]] constexpr const std::stop_token& operator()(auto&&) const noexcept {
            static std::stop_token token;

            return token;
        }

        [[nodiscard]] static consteval bool query(forwarding_query_t) noexcept {
            return true;
        }
    };
    inline constexpr get_stop_token_t get_stop_token{};

    template<typename T>
    using stop_token_of_t = std::remove_cvref_t<decltype(get_stop_token(std::declval<T>()))>;
}

#endif // !EXEC_STOP_TOKEN_HPP