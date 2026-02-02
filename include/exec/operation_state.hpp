#ifndef EXEC_OPERATION_STATE_HPP
#define EXEC_OPERATION_STATE_HPP

#include <concepts>
#include <type_traits>

namespace exec {
    struct operation_state_t {};

    struct start_t {
        template<typename T>
        constexpr auto operator()(T& op) const noexcept {
            op.start();
        }
    };
    inline constexpr start_t start{};

    template <typename T>
    concept operation_state =
        std::derived_from<typename T::operation_state_concept, operation_state_t> &&
        std::is_object_v<T> &&
        requires(T& op) {
            { exec::start(op) } noexcept;
        };
}

#endif // !EXEC_OPERATION_STATE_HPP