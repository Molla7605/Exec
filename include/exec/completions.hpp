#ifndef EXEC_COMPLETIONS_HPP
#define EXEC_COMPLETIONS_HPP

#include <utility>

namespace exec {
    struct set_value_t {
        template<typename ReceiverT, typename... Ts>
        constexpr auto operator()(ReceiverT&& receiver, Ts&&... values) const noexcept {
            return std::forward<ReceiverT>(receiver).set_value(std::forward<Ts>(values)...);
        }
    };
    inline constexpr set_value_t set_value{};

    struct set_error_t {
        template<typename ReceiverT, typename T>
        constexpr auto operator()(ReceiverT&& receiver, T&& value) const noexcept {
            return std::forward<ReceiverT>(receiver).set_error(std::forward<T>(value));
        }
    };
    inline constexpr set_error_t set_error{};

    struct set_stopped_t {
        template<typename ReceiverT>
        constexpr auto operator()(ReceiverT&& receiver) const noexcept {
            return std::forward<ReceiverT>(receiver).set_stopped();
        }
    };
    inline constexpr set_stopped_t set_stopped{};
}

#endif // !EXEC_COMPLETIONS_HPP