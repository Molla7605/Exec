#ifndef EXEC_DETAILS_FORWARD_ENV_HPP
#define EXEC_DETAILS_FORWARD_ENV_HPP

#include "exec/env.hpp"
#include "exec/forwarding_query.hpp"

#include <type_traits>

namespace exec::details {
    template<typename T>
    concept is_forwarding_query = forwarding_query(std::declval<T>());

    template<typename EnvT>
    struct forwarding_env : EnvT {
        template<typename QueryT>
        requires is_forwarding_query<QueryT>
        [[nodiscard]] constexpr decltype(auto) query(QueryT) const noexcept {
            return static_cast<const EnvT&>(*this).query(QueryT{});
        }
    };

    template<typename EnvT>
    forwarding_env(EnvT) -> forwarding_env<EnvT>;

    template<typename>
    struct is_forwarding_env : std::false_type {};

    template<typename EnvT>
    struct is_forwarding_env<forwarding_env<EnvT>> : std::true_type {};

    template<typename T>
    concept valid_forwarding_env = is_forwarding_env<std::remove_cvref_t<T>>::value;

    template<valid_forwarding_env EnvT>
    constexpr decltype(auto) forward_env(EnvT&& env) noexcept {
        return std::forward<EnvT>(env);
    }

    template<typename EnvT>
    constexpr forwarding_env<EnvT> forward_env(EnvT&& env) noexcept {
        return { std::forward<EnvT>(env) };
    }

    template<typename T>
    using forward_env_of_t = decltype(forward_env(std::declval<env_of_t<T>>()));
}

#endif // !EXEC_DETAILS_FORWARD_ENV_HPP