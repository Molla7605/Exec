#ifndef EXEC_RECEIVER_HPP
#define EXEC_RECEIVER_HPP

#include "exec/completion_signatures.hpp"
#include "exec/env.hpp"

#include <concepts>
#include <type_traits>

namespace exec {
    struct receiver_t {};

    template<typename T>
    concept receiver =
        std::derived_from<typename std::remove_cvref_t<T>::receiver_concept, receiver_t> &&
        requires(const std::remove_cvref_t<T>& rcvr) {
            { get_env(rcvr) } -> queryable;
        } &&
        std::is_move_constructible_v<std::remove_cvref_t<T>> &&
        std::constructible_from<std::remove_cvref_t<T>, T>;

    namespace details {
        template<typename SignatureT, typename ReceiverT>
        concept valid_completion_for =
            requires(SignatureT* sig) {
                []<typename TagT, typename... Ts>(TagT(*)(Ts...))
                    requires std::invocable<TagT, std::remove_cvref_t<ReceiverT>, Ts...> {}(sig);
            };

        template<typename ReceiverT, typename SignatureT>
        concept has_completion =
            requires(SignatureT* sig) {
                []<valid_completion_for<ReceiverT>... Ts>(completion_signatures<Ts...>*) {}(sig);
            };
    }

    template<typename ReceiverT, typename CompletionSignaturesT>
    concept receiver_of = receiver<ReceiverT> && details::has_completion<ReceiverT, CompletionSignaturesT>;
}

#endif // !EXEC_RECEIVER_HPP