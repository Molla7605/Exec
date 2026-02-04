#ifndef EXEC_SCOPE_TOKEN_HPP
#define EXEC_SCOPE_TOKEN_HPP

#include "exec/just.hpp"
#include "exec/sender.hpp"

#include <concepts>

namespace exec {
    template<typename T>
    concept scope_token =
        std::copyable<T> &&
        requires(const T token) {
            { token.try_associate() } -> std::same_as<bool>;
            { token.disassociate() } noexcept -> std::same_as<void>;
            { token.wrap(just()) } -> sender_in<env_of_t<std::remove_cvref<decltype(just())>>>;
        };
}

#endif // !EXEC_SCOPE_TOKEN_HPP