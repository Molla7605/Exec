#ifndef EXEC_SENDER_HPP
#define EXEC_SENDER_HPP

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

    struct connect_t {
        template<typename SenderT, typename ReceiverT>
        [[nodiscard]] constexpr decltype(auto) operator()(SenderT&& sender, ReceiverT&& receiver) const
            noexcept(noexcept(std::declval<SenderT>().connect(std::declval<ReceiverT>())))
        {
            return std::forward<SenderT>(sender).connect(std::forward<ReceiverT>(receiver));
        }
    };
    inline constexpr connect_t connect{};

    template<typename SenderT, typename ReceiverT>
    using connect_result_t = decltype(connect(std::declval<SenderT>(), std::declval<ReceiverT>()));

    template<typename SenderT, typename... EnvTs>
    concept sender_in =
        sender<SenderT> &&
        (sizeof...(EnvTs) <= 1) &&
        (queryable<EnvTs> && ...) &&
        details::is_constant<get_completion_signatures<SenderT, EnvTs...>()>;
}

#endif // !EXEC_SENDER_HPP