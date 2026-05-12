#ifndef EXEC_DETAILS_RECEIVER_OF_HPP
#define EXEC_DETAILS_RECEIVER_OF_HPP

#include "exec/completion_signatures.hpp"
#include "exec/receiver.hpp"

#include <concepts>
#include <type_traits>

namespace exec::details {
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

    template<typename ReceiverT, typename CompletionSignaturesT>
    concept receiver_of = receiver<ReceiverT> && details::has_completion<ReceiverT, CompletionSignaturesT>;
}

#endif // !EXEC_DETAILS_RECEIVER_OF_HPP
