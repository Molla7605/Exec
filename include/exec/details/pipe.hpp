#ifndef EXEC_DETAILS_PIPE_HPP
#define EXEC_DETAILS_PIPE_HPP

#include "exec/just.hpp"
#include "exec/sender.hpp"

#include "exec/details/product_type.hpp"

#include <concepts>
#include <type_traits>
#include <utility>

namespace exec {
    template<typename>
    struct sender_adapter_closure;
}

namespace exec::details {
    template<typename T>
    concept pipeable =
        std::derived_from<std::remove_cvref_t<T>, sender_adapter_closure<std::remove_cvref_t<T>>> &&
        std::constructible_from<std::remove_cvref_t<T>, T> &&
        requires(T pipe) {
            { pipe(just()) } -> sender;
        };

    template<typename... PipableTs>
    struct pipe;

    template<typename>
    struct is_pipe : std::false_type {};

    template<typename... PipeableTs>
    struct is_pipe<pipe<PipeableTs...>> : std::true_type {};

    template<typename T>
    concept not_pipe = pipeable<std::remove_cvref_t<T>> && !is_pipe<T>::value;

    template<typename... PipeableTs>
    struct pipe : sender_adapter_closure<pipe<PipeableTs...>> {
        template<pipeable... InnerTs>
        [[nodiscard]] friend constexpr auto operator|(pipe&& self, pipe<InnerTs...>&& other) noexcept {
            return pipe<InnerTs..., PipeableTs...>(
                sender_adapter_closure<pipe<PipeableTs..., InnerTs...>>{},
                merge_product_type(std::forward_like<decltype(self)>(self.adapters), std::forward_like<decltype(other)>(other.adapters))
            );
        }

        template<not_pipe AdapterT>
        [[nodiscard]] friend constexpr auto operator|(pipe&& self, AdapterT&& right) noexcept {
            return pipe<std::remove_cvref_t<AdapterT>, PipeableTs...>(
                sender_adapter_closure<meta_append_front_t<pipe, std::remove_cvref_t<AdapterT>>>{},
                merge_product_type(product_type{ std::forward<AdapterT>(right) }, std::forward_like<decltype(self)>(self.adapters))
            );
        }

        template<not_pipe AdapterT>
        [[nodiscard]] friend constexpr auto operator|(AdapterT&& left, pipe&& self) noexcept {
            return pipe<PipeableTs..., std::remove_cvref_t<AdapterT>>(
                sender_adapter_closure<meta_append_back_t<pipe, std::remove_cvref_t<AdapterT>>>{},
                merge_product_type(std::forward_like<decltype(self)>(self.adapters), product_type{ std::forward<AdapterT>(left) })
            );
        }

        template<sender SenderT, typename AdapterT>
        [[nodiscard]] static constexpr decltype(auto) compose(SenderT&& sender, AdapterT&& adapter) {
            return std::forward<AdapterT>(adapter)(std::forward<SenderT>(sender));
        }

        template<sender SenderT, typename AdapterT, typename... Ts>
        [[nodiscard]] static constexpr decltype(auto) compose(SenderT&& sender, AdapterT&& adapter, Ts&&... others) {
            return std::forward<AdapterT>(adapter)(compose(std::forward<SenderT>(sender), std::forward<Ts>(others)...));
        }

        template<typename Self, sender SenderT>
        [[nodiscard]] constexpr decltype(auto) operator()(this Self&& self, SenderT&& sender) {
            return std::forward_like<Self>(self.adapters).apply([&]<typename... ArgTs>(ArgTs&&... args) mutable {
                return compose(std::forward<SenderT>(sender), std::forward<ArgTs>(args)...);
            });
        }

        product_type<PipeableTs...> adapters;
    };

    template<typename... PipeableTs>
    pipe(sender_adapter_closure<pipe<PipeableTs...>>, product_type<PipeableTs...>) -> pipe<std::decay_t<PipeableTs>...>;

    struct pipe_operator {
        template<sender SenderT, pipeable Self>
        [[nodiscard]] friend constexpr decltype(auto) operator|(SenderT&& sender, Self&& self) noexcept {
            return std::forward<Self>(self)(std::forward<SenderT>(sender));
        }

        template<not_pipe LAdapterT, not_pipe RAdapterT>
        [[nodiscard]] friend constexpr auto operator|(LAdapterT&& left, RAdapterT&& right) noexcept {
            return pipe{
                sender_adapter_closure<pipe<std::remove_cvref_t<RAdapterT>, std::remove_cvref_t<LAdapterT>>>{},
                product_type{ std::forward<RAdapterT>(right), std::forward<LAdapterT>(left) }
            };
        }
    };
}

#endif // !EXEC_DETAILS_PIPE_HPP