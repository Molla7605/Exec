#ifndef EXEC_JUST_HPP
#define EXEC_JUST_HPP

#include "exec/completions.hpp"
#include "exec/completion_signatures.hpp"
#include "exec/operation_state.hpp"
#include "exec/receiver.hpp"
#include "exec/sender.hpp"

#include <tuple>
#include <type_traits>
#include <utility>

namespace exec {
    template<typename TagT, typename ReceiverT, typename... ValueTs>
    struct just_operation_state {
        using operation_state_concept = operation_state_t;

        ReceiverT receiver;
        [[no_unique_address]] std::tuple<ValueTs...> tuple;

        constexpr void start() & noexcept {
            std::apply(
                [&]<typename... Ts>(Ts&&... values) noexcept {
                    TagT{}(std::move(receiver), std::forward<Ts>(values)...);
                },
                std::move(tuple)
            );
        }
    };

    template<typename TagT, typename... ValueTs>
    struct just_sender {
        using sender_concept = sender_t;

        using completion_signatures = completion_signatures<TagT(ValueTs...)>;

        [[no_unique_address]] std::tuple<ValueTs...> tuple;

        template<typename Self, receiver ReceiverT>
        [[nodiscard]] constexpr auto connect(this Self&& self, ReceiverT&& rcvr) ->
            just_operation_state<TagT, std::decay_t<ReceiverT>, ValueTs...>
        {
            return { std::forward<ReceiverT>(rcvr), std::forward_like<Self>(self.tuple) };
        }
    };

    struct just_t {
        template<typename... Ts>
        [[nodiscard]] constexpr auto operator()(Ts&&... values) const ->
            just_sender<set_value_t, std::decay_t<Ts>...>
        {
            return { { std::forward<Ts>(values)... } };
        }
    };
    inline constexpr just_t just{};

    struct just_error_t {
        template<typename T>
        [[nodiscard]] constexpr auto operator()(T&& value) const ->
            just_sender<set_error_t, std::decay_t<T>>
        {
            return { std::forward<T>(value) };
        }
    };
    inline constexpr just_error_t just_error{};

    struct just_stopped_t {
        [[nodiscard]] constexpr auto operator()() const noexcept -> just_sender<set_stopped_t> {
            return {};
        }
    };
    inline constexpr just_stopped_t just_stopped{};
}

#endif // !EXEC_JUST_HPP