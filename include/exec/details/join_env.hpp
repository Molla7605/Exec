#ifndef EXEC_DETAILS_JOIN_ENV_HPP
#define EXEC_DETAILS_JOIN_ENV_HPP

#include "exec/env.hpp"

#include "exec/details/env_traits.hpp"
#include "exec/details/merge_env.hpp"

#include <utility>

namespace exec::details {
    template<valid_env LhsT, not_env RhsT>
    requires (!valid_empty_env<LhsT>)
    [[nodiscard]] constexpr auto join_env(LhsT&& lhs, RhsT&& rhs)
        noexcept(std::is_nothrow_constructible_v<std::remove_cvref_t<LhsT>, LhsT> &&
                 std::is_nothrow_constructible_v<std::remove_cvref_t<RhsT>, RhsT>)
    {
        return std::forward<LhsT>(lhs).apply([&]<typename... Ts>(Ts&&... props) {
            using env_t = meta_append_back_t<std::remove_cvref_t<LhsT>, std::remove_cvref_t<RhsT>>;

            return env_t{ std::forward<Ts>(props)..., std::forward<RhsT>(rhs) };
        });
    }

    template<not_env LhsT, valid_env RhsT>
    requires (!valid_empty_env<RhsT>)
    [[nodiscard]] constexpr auto join_env(LhsT&& lhs, RhsT&& rhs)
        noexcept(std::is_nothrow_constructible_v<std::remove_cvref_t<LhsT>, LhsT> &&
                 std::is_nothrow_constructible_v<std::remove_cvref_t<RhsT>, RhsT>)
    {
        return std::forward<RhsT>(rhs).apply([&]<typename... Ts>(Ts&&... props) {
            using env_t = meta_append_front_t<std::remove_cvref_t<RhsT>, std::remove_cvref_t<LhsT>>;

            return env_t{ std::forward<LhsT>(lhs), std::forward<Ts>(props)... };
        });
    }

    template<valid_empty_env LhsT, typename RhsT>
    [[nodiscard]] constexpr decltype(auto) join_env(LhsT&&, RhsT&& rhs) noexcept {
        return std::forward<RhsT>(rhs);
    }

    template<typename LhsT, valid_empty_env RhsT>
    [[nodiscard]] constexpr decltype(auto) join_env(LhsT&& lhs, RhsT&&) noexcept {
        return std::forward<LhsT>(lhs);
    }

    template<valid_env LhsT, valid_env RhsT>
    [[nodiscard]] constexpr auto join_env(LhsT&& lhs, RhsT&& rhs)
        noexcept(std::is_nothrow_constructible_v<std::remove_cvref_t<LhsT>, RhsT> &&
                 std::is_nothrow_constructible_v<std::remove_cvref_t<RhsT>, RhsT>)
    {
        return merge_env(std::forward<LhsT>(lhs), std::forward<RhsT>(rhs));
    }

    template<typename LhsT, typename RhsT>
    [[nodiscard]] constexpr auto join_env(LhsT&& lhs, RhsT&& rhs)
        noexcept(std::is_nothrow_constructible_v<std::remove_cvref_t<LhsT>, RhsT> &&
                 std::is_nothrow_constructible_v<std::remove_cvref_t<RhsT>, RhsT>)
    {
        return env{ std::forward<LhsT>(lhs), std::forward<RhsT>(rhs) };
    }
}

#endif // !EXEC_DETAILS_JOIN_ENV_HPP