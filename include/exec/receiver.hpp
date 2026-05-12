#ifndef EXEC_RECEIVER_HPP
#define EXEC_RECEIVER_HPP

#include "exec/completion_signatures.hpp"
#include "exec/env.hpp"

#include <concepts>
#include <type_traits>

namespace exec {
    struct receiver_tag {};

    template<typename T>
    concept receiver =
        std::derived_from<typename std::remove_cvref_t<T>::receiver_concept, receiver_tag> &&
        requires(const std::remove_cvref_t<T>& rcvr) {
            { get_env(rcvr) } -> queryable;
        } &&
        std::is_move_constructible_v<std::remove_cvref_t<T>> &&
        std::constructible_from<std::remove_cvref_t<T>, T>;


}

#endif // !EXEC_RECEIVER_HPP