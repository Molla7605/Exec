#ifndef EXEC_DETAILS_PIPE_HPP
#define EXEC_DETAILS_PIPE_HPP

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
    concept pipeable = std::derived_from<T, sender_adapter_closure<T>>;

    template<pipeable... PipeableTs>
    struct pipe : sender_adapter_closure<pipe<PipeableTs...>> {
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
    pipe(PipeableTs...) -> pipe<std::decay_t<PipeableTs>...>;

    struct pipe_operator {
        template<sender SenderT, pipeable Self>
        [[nodiscard]] friend constexpr decltype(auto) operator|(SenderT&& sender, Self&& self) noexcept {
            return std::forward<Self>(self)(std::forward<SenderT>(sender));
        }

        template<pipeable LAdapterT, pipeable RAdapterT>
        [[nodiscard]] friend constexpr auto operator|(LAdapterT&& left, RAdapterT&& right) noexcept {
            return pipe{ {}, std::forward<LAdapterT>(left), std::forward<RAdapterT>(right) };
        }
    };
}

#endif // !EXEC_DETAILS_PIPE_HPP