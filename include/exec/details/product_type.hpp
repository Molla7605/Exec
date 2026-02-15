#ifndef EXEC_DETAILS_PRODUCT_TYPE_HPP
#define EXEC_DETAILS_PRODUCT_TYPE_HPP

#include "exec/details/meta_index.hpp"

#include <type_traits>
#include <utility>

namespace exec::details {
    template<typename...>
    struct product_type_impl {};

    template<std::size_t INDEX, typename T>
    struct product_type_impl<std::integral_constant<std::size_t, INDEX>, T> {
        [[no_unique_address]] T value;
    };

    template<std::size_t... INDICES, typename... Ts>
    struct product_type_impl<std::index_sequence<INDICES...>, Ts...> : product_type_impl<std::integral_constant<std::size_t, INDICES>, Ts>... {
        template<std::size_t INDEX, typename Self>
        requires (INDEX < sizeof...(Ts))
        [[nodiscard]] constexpr decltype(auto) get(this Self&& self) noexcept {
            return std::forward_like<Self>(
                self.template product_type_impl<std::integral_constant<std::size_t, INDEX>, meta_index_t<INDEX, Ts...>>::value
            );
        }

        template<typename InvocableT, typename Self>
        requires std::invocable<InvocableT, Ts...>
        [[nodiscard]] constexpr decltype(auto) apply(this Self&& self, InvocableT&& invocable)
            noexcept(std::is_nothrow_invocable_v<InvocableT, Ts...>)
        {
            return std::invoke(
                std::forward<InvocableT>(invocable),
                std::forward_like<Self>(self.template product_type_impl<std::integral_constant<std::size_t, INDICES>, Ts>::value)...
            );
        }
    };

    template<typename... Ts>
    struct product_type : product_type_impl<std::make_index_sequence<sizeof...(Ts)>, Ts...> {};

    template<typename... Ts>
    product_type(Ts&&...) -> product_type<std::decay_t<Ts>...>;
}

#endif // !EXEC_DETAILS_PRODUCT_TYPE_HPP