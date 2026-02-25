#ifndef EXEC_DETAILS_JOIN_ENV_HPP
#define EXEC_DETAILS_JOIN_ENV_HPP

#include "exec/queryable.hpp"

#include <concepts>
#include <utility>

namespace exec::details {
    template<queryable LEnv, queryable REnv>
    struct joined_env : LEnv, REnv {
        template<typename QueryT>
        [[nodiscard]] constexpr decltype(auto) query(QueryT) const noexcept {
            if constexpr (requires { std::declval<const LEnv&>().query(QueryT{}); }) {
                return static_cast<const LEnv&>(*this).query(QueryT{});
            }
            else {
                return static_cast<const REnv&>(*this).query(QueryT{});
            }
        }
    };

    template<typename LEnv, typename REnv>
    joined_env(LEnv, REnv) -> joined_env<LEnv, REnv>;

    template<typename LEnv, typename REnv>
    requires (!(std::derived_from<LEnv, empty_env> || std::derived_from<REnv, empty_env>) &&
              (std::derived_from<LEnv, REnv> || std::derived_from<REnv, LEnv>))
    [[nodiscard]] constexpr decltype(auto) join_env(LEnv&& l_env, REnv&& r_env) noexcept {
        if constexpr (valid_forwarding_env<LEnv>) {
            return std::forward<LEnv>(l_env);
        }
        else {
            return std::forward<REnv>(r_env);
        }
    }

    template<typename LEnv, typename REnv>
    requires std::derived_from<LEnv, empty_env>
    [[nodiscard]] constexpr decltype(auto) join_env(LEnv&& l_env, REnv&& r_env) noexcept {
        return std::forward<REnv>(r_env);
    }

    template<typename LEnv, typename REnv>
    requires std::derived_from<REnv, empty_env>
    [[nodiscard]] constexpr decltype(auto) join_env(LEnv&& l_env, REnv&& r_env) noexcept {
        return std::forward<LEnv>(l_env);
    }

    template<typename LEnv, typename REnv>
    [[nodiscard]] constexpr joined_env<std::decay_t<LEnv>, std::decay_t<REnv>> join_env(LEnv&& l_env, REnv&& r_env)
        noexcept(std::is_nothrow_constructible_v<joined_env<std::decay_t<LEnv>, std::decay_t<REnv>>, LEnv, REnv>)
    {
        return { std::forward<LEnv>(l_env), std::forward<REnv>(r_env) };
    }
}

#endif // !EXEC_DETAILS_JOIN_ENV_HPP