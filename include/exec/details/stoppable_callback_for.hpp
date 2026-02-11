#ifndef EXEC_DETAILS_STOPPABLE_CALLBACK_FOR_HPP
#define EXEC_DETAILS_STOPPABLE_CALLBACK_FOR_HPP

#include <concepts>

namespace exec::details {
    template<typename CallbackT, typename TokenT, typename InitT = CallbackT>
    concept stoppable_callback_for =
        std::invocable<CallbackT> &&
        std::constructible_from<CallbackT, InitT> &&
        requires { typename TokenT::template callback_type<CallbackT>; } &&
        std::constructible_from<typename TokenT::template callback_type<CallbackT>, const TokenT&, InitT>;
}

#endif // !EXEC_DETAILS_STOPPABLE_CALLBACK_FOR_HPP