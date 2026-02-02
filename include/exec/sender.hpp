#ifndef EXEC_SENDER_HPP
#define EXEC_SENDER_HPP

#include "exec/completion_signatures.hpp"
#include "exec/env.hpp"
#include "exec/query.hpp"
#include "exec/receiver.hpp"

#include "exec/details/valid_completion_signatures.hpp"

namespace exec {
    struct sender_t {};

    template<typename T>
    concept sender =
        std::derived_from<typename std::remove_cvref_t<T>::sender_concept, sender_t> &&
        requires(const std::remove_cvref_t<T>& sndr) {
            { get_env(sndr) } -> queryable;
        } &&
        std::is_move_constructible_v<std::remove_cvref_t<T>> &&
        std::constructible_from<std::remove_cvref_t<T>, T>;

    struct connect_t {
        template<typename SenderT, typename ReceiverT>
        [[nodiscard]] constexpr auto operator()(SenderT&& sender, ReceiverT&& receiver) const {
            return std::forward<SenderT>(sender).connect(std::forward<ReceiverT>(receiver));
        }
    };
    inline constexpr connect_t connect{};

    template<typename SenderT, typename ReceiverT>
    using connect_result_t = decltype(connect(std::declval<SenderT>(), std::declval<ReceiverT>()));

    template<typename SenderT, typename EnvT = empty_env>
    concept sender_in =
        sender<SenderT> &&
        queryable<EnvT> &&
        requires(SenderT&& sndr, EnvT&& env) {
            { get_completion_signatures(std::forward<SenderT>(sndr), std::forward<EnvT>(env)) } ->
                details::valid_completion_signatures;
        };

    template<typename SenderT, typename ReceiverT>
    concept sender_to =
        sender_in<SenderT, env_of_t<ReceiverT>> &&
        receiver_of<ReceiverT, completion_signatures_of_t<SenderT, env_of_t<ReceiverT>>> &&
        requires(SenderT&& sndr, ReceiverT&& rcvr) {
            connect(std::forward<SenderT>(sndr), std::forward<ReceiverT>(rcvr));
        };
}

#endif // !EXEC_SENDER_HPP