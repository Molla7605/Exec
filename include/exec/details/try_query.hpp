#ifndef EXEC_DETAILS_TRY_QUERY_HPP
#define EXEC_DETAILS_TRY_QUERY_HPP

#include "exec/details/env_traits.hpp"

namespace exec::details {
    template<typename EnvT, typename TagT, typename... ArgTs>
    requires has_query<const EnvT&, TagT, ArgTs...>
    [[nodiscard]] constexpr decltype(auto) try_query(const EnvT& env, TagT, ArgTs&&... args) noexcept {
        return env.query(TagT{}, std::forward<TagT>(args)...);
    }
    
    template<typename EnvT, typename TagT, typename... ArgTs>
    [[nodiscard]] constexpr decltype(auto) try_query(const EnvT& env, TagT, ArgTs&&...) noexcept {
        return env.query(TagT{});
    }
}

#endif // !EXEC_DETAILS_TRY_QUERY_HPP
