#ifndef EXEC_SCOPE_TOKEN_HPP
#define EXEC_SCOPE_TOKEN_HPP

#include "exec/just.hpp"
#include "exec/scope_association.hpp"
#include "exec/sender.hpp"

#include <concepts>

namespace exec {
    template<typename T>
    concept scope_token =
        std::copyable<T> &&
        requires(const T token) {
            { token.try_associate() } -> scope_association;
            { token.wrap(just()) } -> sender_in<env_of_t<std::remove_cvref<decltype(just())>>>;
        };
}

#endif // !EXEC_SCOPE_TOKEN_HPP