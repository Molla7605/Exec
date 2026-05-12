#ifndef EXEC_DETAILS_ENV_TRAITS_HPP
#define EXEC_DETAILS_ENV_TRAITS_HPP

#include "exec/queryable.hpp"

#include "exec/details/type_list.hpp"

#include <type_traits>
#include <utility>

namespace exec {
    template<queryable... PropTs>
    class env;
}

namespace exec::details {
    template<typename EnvT, typename TagT, typename... ArgTs>
    concept has_query =
        requires(const EnvT& env, ArgTs&&... args) {
            env.query(TagT{}, std::forward<ArgTs>(args)...);
        };

    template<typename>
    struct is_env : std::false_type {};

    template<typename... PropTs>
    struct is_env<env<PropTs...>> : std::true_type {};

    template<typename T>
    concept valid_env = is_env<std::remove_cvref_t<T>>::value;

    template<typename T>
    concept not_env = !is_env<std::remove_cvref_t<T>>::value;

    template<typename T>
    concept valid_empty_env = valid_env<T> && is_empty_list_v<std::remove_cvref_t<T>>;
}

#endif // !EXEC_DETAILS_ENV_TRAITS_HPP
