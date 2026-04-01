#ifndef EXEC_DETAILS_PRODUCT_TYPE_HPP
#define EXEC_DETAILS_PRODUCT_TYPE_HPP

#include "exec/details/meta_index.hpp"
#include "exec/details/meta_add.hpp"

#include <functional>
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
        template<size_t INDEX>
        using child_t = product_type_impl<std::integral_constant<std::size_t, INDEX>, meta_index_t<INDEX, Ts...>>;

        template<std::size_t INDEX, typename Self>
        requires (INDEX < sizeof...(Ts))
        [[nodiscard]] constexpr decltype(auto) get(this Self&& self) noexcept {
            return std::forward_like<Self>(
                self.child_t<INDEX>::value
            );
        }

        template<typename InvocableT, typename Self>
        [[nodiscard]] constexpr decltype(auto) apply(this Self&& self, InvocableT&& invocable)
            noexcept(std::is_nothrow_invocable_v<InvocableT, decltype(std::forward_like<Self>(std::declval<Ts>()))...>)
        {
            return std::invoke(
                std::forward<InvocableT>(invocable),
                std::forward_like<Self>(self.child_t<INDICES>::value)...
            );
        }
    };

    template<typename... Ts>
    struct product_type : product_type_impl<std::make_index_sequence<sizeof...(Ts)>, Ts...> {
        static constexpr std::size_t SIZE = sizeof...(Ts);
    };

    template<typename... Ts>
    product_type(Ts&&...) -> product_type<std::decay_t<Ts>...>;

    template<typename LeftT, typename RightT>
    [[nodiscard]] constexpr auto merge_product_type(LeftT&& left, RightT&& right)
        noexcept(std::is_nothrow_constructible_v<std::decay_t<LeftT>, LeftT> &&
                 std::is_nothrow_constructible_v<std::decay_t<RightT>, RightT>)
    {
        return [&]<std::size_t... L_INDICES, std::size_t... R_INDICES>(std::index_sequence<L_INDICES...>, std::index_sequence<R_INDICES...>) {
            return meta_add_t<std::decay_t<LeftT>, std::decay_t<RightT>>{
                std::forward<LeftT>(left).template get<L_INDICES>()...,
                std::forward<RightT>(right).template get<R_INDICES>()...
            };
        }(std::make_index_sequence<std::remove_cvref_t<LeftT>::SIZE>{},
          std::make_index_sequence<std::remove_cvref_t<RightT>::SIZE>{});
    }
}

#endif // !EXEC_DETAILS_PRODUCT_TYPE_HPP