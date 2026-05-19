#ifndef EXEC_DETAILS_HIDE_SCHED_HPP
#define EXEC_DETAILS_HIDE_SCHED_HPP

#include "exec/details/env_traits.hpp"

#include <concepts>
#include <type_traits>
#include <utility>

namespace exec {
    struct get_scheduler_t;
    struct get_domain_t;
}

namespace exec::details {
    template<typename EnvT>
    struct hide_scheduler_env : EnvT {
        template<typename TagT, typename... ArgTs>
        requires (!std::same_as<TagT, get_domain_t> && !std::same_as<TagT, get_scheduler_t>)
        [[nodiscard]] constexpr decltype(auto) query(TagT tag, ArgTs&&... args) const noexcept {
            return EnvT::query(tag, std::forward<ArgTs>(args)...);
        }
    };

    template<typename EnvT>
    hide_scheduler_env(EnvT) -> hide_scheduler_env<std::decay_t<EnvT>>;

    template<typename EnvT>
    requires (!std::same_as<std::decay_t<EnvT>, hide_scheduler_env<std::decay_t<EnvT>>>) &&
             (has_query<EnvT, get_scheduler_t> ||
              has_query<EnvT, get_domain_t>)
    [[nodiscard]] constexpr auto hide_sched(EnvT&& env)
        noexcept(std::is_nothrow_constructible_v<hide_scheduler_env<std::decay_t<EnvT>>, EnvT>)
    {
        return hide_scheduler_env{ std::forward<EnvT>(env) };
    }

    template<typename EnvT>
    [[nodiscard]] constexpr decltype(auto) hide_sched(EnvT&& env) noexcept {
        return std::forward<EnvT>(env);
    }
}

#endif // !EXEC_DETAILS_HIDE_SCHED_HPP
