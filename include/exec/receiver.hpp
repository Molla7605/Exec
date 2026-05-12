#ifndef EXEC_RECEIVER_HPP
#define EXEC_RECEIVER_HPP

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
        std::move_constructible<std::remove_cvref_t<T>> &&
        std::constructible_from<std::remove_cvref_t<T>, T> &&
        std::is_nothrow_move_constructible_v<std::remove_cvref_t<T>>;

    template<typename T, typename ChildOperationT>
    concept inline_receiver =
        receiver<T> &&
        requires (ChildOperationT* child) {
            { std::remove_cvref_t<T>::make_receiver_for(child) } noexcept ->
                std::same_as<std::remove_cvref_t<T>>;
        };

}

#endif // !EXEC_RECEIVER_HPP