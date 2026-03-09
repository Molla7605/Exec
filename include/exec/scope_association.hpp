#ifndef EXEC_SCOPE_ASSOCIATION_HPP
#define EXEC_SCOPE_ASSOCIATION_HPP

#include <concepts>
#include <type_traits>

namespace exec {
    template<typename T>
    concept scope_association =
        std::movable<T> &&
        std::is_nothrow_move_constructible_v<T> &&
        std::is_nothrow_move_assignable_v<T> &&
        std::default_initializable<T> &&
        requires(const T assoc) {
            { static_cast<bool>(assoc) } noexcept;
            { assoc.try_associate() } -> std::same_as<T>;
        };
}

#endif // !EXEC_SCOPE_ASSOCIATION_HPP