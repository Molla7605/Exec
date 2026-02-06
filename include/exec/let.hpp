#ifndef EXEC_LET_HPP
#define EXEC_LET_HPP

#include "exec/completions.hpp"
#include "exec/completion_signatures.hpp"
#include "exec/env.hpp"
#include "exec/operation_state.hpp"
#include "exec/pipe_adapter.hpp"
#include "exec/receiver.hpp"
#include "exec/sender.hpp"

#include "exec/details/meta_bind.hpp"

#include <exception>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace exec {
    template<typename OpT, typename ReceiverT>
    struct let_receiver {
        using receiver_concept = receiver_t;

        OpT* op;

        void set_value(auto&&... values) && noexcept {
            op->template process<set_value_t>(std::forward<decltype(values)>(values)...);
        }

        void set_error(auto&& value) && noexcept {
            op->template process<set_error_t>(std::forward<decltype(value)>(value));
        }

        void set_stopped() && noexcept {
            op->template process<set_stopped_t>();
        }

        [[nodiscard]] constexpr forward_env_of_t<ReceiverT> query(get_env_t) const noexcept {
            return forward_env(get_env(op->receiver));
        }
    };

    namespace details {
        template<template<typename...> typename TypeListT, typename EnvT>
        struct bind_env_wrapper {
            template<typename... Ts>
            using type = TypeListT<EnvT, Ts...>;
        };
    }

    template<typename TagT, typename SenderT, typename ReceiverT, typename FnT>
    struct let_operation_state {
        using operation_state_concept = operation_state_t;

        template<typename... Ts>
        using op_result_t = std::decay_t<connect_result_t<std::invoke_result_t<FnT, Ts...>, ReceiverT>>;

        using first_op_t = std::decay_t<connect_result_t<SenderT, let_receiver<let_operation_state, ReceiverT>>>;
        using second_op_variant_t = value_types_of_t<SenderT, env_of_t<ReceiverT>, op_result_t, std::variant>;

        using state_t = details::meta_add_t<std::variant<SenderT, first_op_t>, second_op_variant_t>;

        using args_t = details::meta_add_t<std::variant<std::monostate>,
                                           details::gather_signatures<TagT,
                                                                      completion_signatures_of_t<SenderT, env_of_t<ReceiverT>>,
                                                                      details::decayed_tuple,
                                                                      std::variant>>;

        ReceiverT receiver;
        FnT fn;
        [[no_unique_address]] state_t state;
        [[no_unique_address]] args_t args;

        template<typename T>
        requires std::is_same_v<TagT, T>
        void process(auto&&... values) & noexcept {
            try {
                using result_t = details::decayed_tuple<decltype(values)...>;

                auto& op =
                    state.template emplace<op_result_t<decltype(values)...>>(
                        exec::connect(
                            std::apply(std::move(fn), args.template emplace<result_t>(std::forward<decltype(values)>(values)...)),
                            std::move(receiver)
                        )
                    );

                exec::start(op);
            }
            catch (...) {
                exec::set_error(std::move(receiver), std::current_exception());
            }
        }

        template<typename T>
        void process(auto&&... values) & noexcept {
            T{}(std::move(receiver), std::forward<decltype(values)>(values)...);
        }

        void start() & noexcept {
            first_op_t* op = nullptr;
            try {
                op = std::addressof(state.template emplace<first_op_t>(
                                       exec::connect(std::get<SenderT>(state), let_receiver<let_operation_state, ReceiverT>{ this })
                                    ));
            }
            catch (...) {
                exec::set_error(std::move(receiver), std::current_exception());
            }

            if (op) {
                exec::start(*op);
            }
        }
    };

    template<typename TagT, typename SenderT, typename FnT>
    struct let_sender {
        using sender_concept = sender_t;

        SenderT sender;
        FnT fn;

        template<typename EnvT, typename... ArgTs>
        requires std::invocable<FnT, ArgTs...>
        using signature_t = completion_signatures_of_t<std::invoke_result_t<FnT, ArgTs...>, EnvT>;

        template<typename EnvT>
        requires std::is_same_v<TagT, set_value_t>
        [[nodiscard]] constexpr auto get_completion_signatures(const EnvT&) const noexcept {
            return transform_completion_signatures_of<SenderT,
                                                      EnvT,
                                                      completion_signatures<set_error_t(std::exception_ptr)>,
                                                      details::meta_bind_front<signature_t, EnvT>::template type>{};
        }

        template<typename EnvT>
        requires std::is_same_v<TagT, set_error_t>
        [[nodiscard]] constexpr auto get_completion_signatures(const EnvT&) const noexcept {
            return transform_completion_signatures_of<SenderT,
                                                      EnvT,
                                                      completion_signatures<set_error_t(std::exception_ptr)>,
                                                      details::default_set_value_t,
                                                      details::meta_bind_front<signature_t, EnvT>::template type>{};
        }

        template<typename EnvT>
        requires std::is_same_v<TagT, set_stopped_t>
        [[nodiscard]] constexpr auto get_completion_signatures(const EnvT&) const noexcept {
            return transform_completion_signatures_of<SenderT,
                                                      EnvT,
                                                      completion_signatures<set_error_t(std::exception_ptr)>,
                                                      details::default_set_value_t,
                                                      details::default_set_error_t,
                                                      signature_t<EnvT>>{};
        }

        [[nodiscard]] constexpr decltype(auto) query(get_env_t) const noexcept {
            return forward_env(get_env(sender));
        }

        template<typename Self, receiver ReceiverT>
        [[nodiscard]] constexpr auto connect(this Self&& self, ReceiverT&& rcvr) ->
            let_operation_state<TagT, SenderT, std::decay_t<ReceiverT>, FnT>
        {
            return { std::forward<ReceiverT>(rcvr), std::forward_like<Self>(self.fn), { std::forward_like<Self>(self.sender) }, {} };
        }
    };

    struct let_value_t {
        [[nodiscard]] constexpr auto operator()(sender auto&& input, auto&& invocable) const ->
            let_sender<set_value_t, std::decay_t<decltype(input)>, std::decay_t<decltype(invocable)>>
        {
            return { std::forward<decltype(input)>(input), std::forward<decltype(invocable)>(invocable) };
        }

        [[nodiscard]] constexpr auto operator()(auto&& invocable) const {
            return pipe_adapter {
                []<typename... Ts>(Ts&&... args) constexpr {
                    return let_sender<set_value_t, Ts...>{ std::forward<Ts>(args)... };
                },
                std::make_tuple(std::forward<decltype(invocable)>(invocable))
            };
        }
    };
    inline constexpr let_value_t let_value{};

    struct let_error_t {
        [[nodiscard]] constexpr auto operator()(sender auto&& input, auto&& invocable) const ->
            let_sender<set_error_t, std::decay_t<decltype(input)>, std::decay_t<decltype(invocable)>>
        {
            return { std::forward<decltype(input)>(input), std::forward<decltype(invocable)>(invocable) };
        }

        [[nodiscard]] constexpr auto operator()(auto&& invocable) const {
            return pipe_adapter {
                []<typename... Ts>(Ts&&... args) constexpr {
                    return let_sender<set_error_t, Ts...>{ std::forward<Ts>(args)... };
                },
                std::make_tuple(std::forward<decltype(invocable)>(invocable))
            };
        }
    };
    inline constexpr let_error_t let_error{};

    struct let_stopped_t {
        [[nodiscard]] constexpr auto operator()(sender auto&& input, std::invocable auto&& invocable) const ->
            let_sender<set_stopped_t, std::decay_t<decltype(input)>, std::decay_t<decltype(invocable)>>
        {
            return { std::forward<decltype(input)>(input), std::forward<decltype(invocable)>(invocable) };
        }

        [[nodiscard]] constexpr auto operator()(std::invocable auto&& invocable) const {
            return pipe_adapter {
                []<typename... Ts>(Ts&&... args) constexpr {
                    return let_sender<set_stopped_t, Ts...>{ std::forward<Ts>(args)... };
                },
                std::make_tuple(std::forward<decltype(invocable)>(invocable))
            };
        }
    };
    inline constexpr let_stopped_t let_stopped{};
}

#endif // !EXEC_LET_HPP