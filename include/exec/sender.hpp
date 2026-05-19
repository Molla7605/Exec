#ifndef EXEC_SENDER_HPP
#define EXEC_SENDER_HPP

#include "exec/completion_signatures.hpp"
#include "exec/env.hpp"
#include "exec/queryable.hpp"

#include "exec/details/indirect_meta_apply.hpp"

namespace exec {
    struct sender_tag {};

    template<typename T>
    concept sender =
        std::derived_from<typename std::remove_cvref_t<T>::sender_concept, sender_tag> &&
        requires(const std::remove_cvref_t<T>& sndr) {
            { get_env(sndr) } -> queryable;
        } &&
        std::is_move_constructible_v<std::remove_cvref_t<T>> &&
        std::constructible_from<std::remove_cvref_t<T>, T>;

    template<typename SenderT, typename... EnvTs>
    concept sender_in =
        sender<SenderT> &&
        (sizeof...(EnvTs) <= 1) &&
        (queryable<EnvTs> && ...) &&
        details::is_constant<get_completion_signatures<SenderT, EnvTs...>()>;

    template<typename T>
    using tag_of_t = std::remove_cvref_t<decltype(std::declval<T>().template get<0>())>;
}

#endif // !EXEC_SENDER_HPP