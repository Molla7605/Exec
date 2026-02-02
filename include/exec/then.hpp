#ifndef EXEC_THEN_HPP
#define EXEC_THEN_HPP

#include "exec/completions.hpp"
#include "exec/completion_signatures.hpp"
#include "exec/env.hpp"
#include "exec/pipe_adapter.hpp"
#include "exec/receiver.hpp"
#include "exec/sender.hpp"
#include "exec/transform_completion_signatures.hpp"

#include <concepts>
#include <exception>
#include <tuple>
#include <type_traits>
#include <utility>

namespace exec {
    template<typename TagT, receiver ReceiverT, typename FnT>
    struct then_receiver {
        using receiver_concept = receiver_t;

        ReceiverT receiver;
        FnT fn;

        void set_value(auto&&... values) && noexcept {
            process<set_value_t>(std::forward<decltype(values)>(values)...);
        }

        void set_error(auto&& value) && noexcept {
            process<set_error_t>(std::forward<decltype(value)>(value));
        }

        void set_stopped() && noexcept {
            process<set_stopped_t>();
        }

        [[nodiscard]] constexpr decltype(auto) query(get_env_t) const noexcept {
            return forward_env(get_env(receiver));
        }

    private:
        template<typename T>
        requires std::is_same_v<TagT, T>
        void process(this auto&& self, auto&&... values) noexcept {
            try {
                if constexpr (std::is_void_v<std::invoke_result_t<FnT, decltype(values)...>>) {
                    std::invoke(std::forward_like<decltype(self)>(self.fn), std::forward<decltype(values)>(values)...);
                    exec::set_value(std::move(self.receiver));
                }
                else {
                    exec::set_value(std::move(self.receiver),
                                    std::invoke(std::forward_like<decltype(self)>(self.fn), std::forward<decltype(values)>(values)...));
                }
            }
            catch (...) {
                exec::set_error(std::move(self.receiver), std::current_exception());
            }
        }

        template<typename T>
        void process(this auto&& self, auto&&... values) noexcept {
            T{}(std::move(self.receiver), std::forward<decltype(values)>(values)...);
        }
    };

    template<typename TagT, sender SenderT, typename FnT>
    struct then_sender {
        using sender_concept = sender_t;

        SenderT sender;
        FnT fn;

        template<typename... Ts>
        requires std::invocable<FnT, Ts...>
        using signature =
            completion_signatures<std::conditional_t<std::is_void_v<std::invoke_result_t<FnT, Ts...>>,
                                                     set_value_t(),
                                                     set_value_t(std::invoke_result_t<FnT, Ts...>)>>;

        template<typename EnvT>
        requires std::is_same_v<TagT, set_value_t>
        [[nodiscard]] constexpr auto get_completion_signatures(const EnvT&) const noexcept {
            return transform_completion_signatures_of<SenderT,
                                                      EnvT,
                                                      completion_signatures<set_error_t(std::exception_ptr)>,
                                                      signature>{};
        }

        template<typename EnvT>
        requires std::is_same_v<TagT, set_error_t>
        [[nodiscard]] constexpr auto get_completion_signatures(const EnvT&) const noexcept {
            return transform_completion_signatures_of<SenderT,
                                                      EnvT,
                                                      completion_signatures<set_error_t(std::exception_ptr)>,
                                                      details::default_set_value_t,
                                                      signature>{};
        }

        template<typename EnvT>
        requires std::is_same_v<TagT, set_stopped_t>
        [[nodiscard]] constexpr auto get_completion_signatures(const EnvT&) const noexcept {
            return transform_completion_signatures_of<SenderT,
                                                      EnvT,
                                                      completion_signatures<set_error_t(std::exception_ptr)>,
                                                      details::default_set_value_t,
                                                      details::default_set_error_t,
                                                      signature<>>{};
        }

        [[nodiscard]] constexpr decltype(auto) query(get_env_t) const noexcept {
            return forward_env(get_env(sender));
        }

        [[nodiscard]] constexpr auto connect(this auto&& self, receiver auto&& rcvr) {
            return exec::connect(std::forward_like<decltype(self)>(self.sender),
                                 then_receiver<TagT, std::decay_t<decltype(rcvr)>, FnT>{
                                      std::forward<decltype(rcvr)>(rcvr),
                                      std::forward_like<decltype(self)>(self.fn)
                                 });
        }
    };

    struct then_t {
        [[nodiscard]] constexpr auto operator()(sender auto&& input, auto&& invocable) const ->
            then_sender<set_value_t, std::decay_t<decltype(input)>, std::decay_t<decltype(invocable)>>
        {
            return { std::forward<decltype(input)>(input), std::forward<decltype(invocable)>(invocable) };
        }

        [[nodiscard]] constexpr auto operator()(auto&& invocable) const {
            return pipe_adapter {
                []<typename... Ts>(Ts&&... args) constexpr {
                    return then_sender<set_value_t, std::decay_t<Ts>...>{ std::forward<Ts>(args)... };
                }, std::make_tuple(std::forward<decltype(invocable)>(invocable))
            };
        }
    };
    inline constexpr then_t then{};

    struct upon_error_t {
        [[nodiscard]] constexpr auto operator()(sender auto&& input, auto&& invocable) const ->
            then_sender<set_error_t, std::decay_t<decltype(input)>, std::decay_t<decltype(invocable)>>
        {
            return { std::forward<decltype(input)>(input), std::forward<decltype(invocable)>(invocable) };
        }

        [[nodiscard]] constexpr auto operator()(auto&& invocable) const {
            return pipe_adapter {
                []<typename... Ts>(Ts&&... args) constexpr {
                    return then_sender<set_error_t, std::decay_t<Ts>...>{ std::forward<Ts>(args)... };
                }, std::make_tuple(std::forward<decltype(invocable)>(invocable))
            };
        }
    };
    inline constexpr upon_error_t upon_error{};

    struct upon_stopped_t {
        [[nodiscard]] constexpr auto operator()(sender auto&& input, std::invocable auto&& invocable) const ->
            then_sender<set_stopped_t, std::decay_t<decltype(input)>, std::decay_t<decltype(invocable)>>
        {
            return { std::forward<decltype(input)>(input), std::forward<decltype(invocable)>(invocable) };
        }

        [[nodiscard]] constexpr auto operator()(std::invocable auto&& invocable) const {
            return pipe_adapter {
                []<typename... Ts>(Ts&&... args) constexpr {
                    return then_sender<set_stopped_t, std::decay_t<Ts>...>{ std::forward<Ts>(args)... };
                }, std::make_tuple(std::forward<decltype(invocable)>(invocable))
            };
        }
    };
    inline constexpr upon_stopped_t upon_stopped{};
}

#endif // !EXEC_THEN_HPP