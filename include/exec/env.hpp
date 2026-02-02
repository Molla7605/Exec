#ifndef EXEC_ENV_HPP
#define EXEC_ENV_HPP

#include <exec/query.hpp>

#include <concepts>
#include <utility>

namespace exec {
    namespace details {
        template<typename QueryT>
        concept is_forwarding_query = forwarding_query(QueryT{});
    }

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

    template<typename EnvT>
    struct forwarding_env : EnvT {
        template<typename QueryT>
        requires details::is_forwarding_query<QueryT>
        [[nodiscard]] constexpr decltype(auto) query(QueryT) const noexcept {
            return static_cast<const EnvT&>(*this).query(QueryT{});
        }
    };

    template<typename EnvT>
    forwarding_env(EnvT) -> forwarding_env<EnvT>;

    template<typename EnvT>
    constexpr forwarding_env<EnvT>& forward_env(forwarding_env<EnvT>& env) noexcept {
        return env;
    }

    template<typename EnvT>
    constexpr const forwarding_env<EnvT>& forward_env(const forwarding_env<EnvT>& env) noexcept {
        return env;
    }

    template<typename EnvT>
    constexpr forwarding_env<EnvT> forward_env(EnvT&& env) noexcept {
        return { std::forward<EnvT>(env) };
    }

    template<typename T>
    using forward_env_of_t = decltype(forward_env(std::declval<env_of_t<T>>()));
}

#endif // !EXEC_ENV_HPP