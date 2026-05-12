#ifndef EXEC_DETAILS_MERGE_ENV_HPP
#define EXEC_DETAILS_MERGE_ENV_HPP

#include "exec/env.hpp"

#include "exec/details/meta_function.hpp"
#include "exec/details/type_holder.hpp"
#include "exec/details/type_list.hpp"

#include <type_traits>
#include <utility>

namespace exec::details {
    template<valid_type_holder LhsListT, valid_type_holder RhsListT>
    [[nodiscard]] consteval auto make_unique_env_indices() noexcept {
        static constexpr auto get_env_size = []<typename... Ts>(type_holder<Ts...>) constexpr noexcept {
            return sizeof...(Ts);
        };

        [[maybe_unused]]
        static constexpr auto to_constants = []<std::size_t... INDICES>(std::index_sequence<INDICES...>) constexpr noexcept {
            return type_holder<std::integral_constant<std::size_t, INDICES>...>{};
        };

        [[maybe_unused]]
        static constexpr auto make_index_sequence = []<typename T>(T) constexpr noexcept {
            using result_t = elements_of<T>::template apply<std::index_sequence_for>;

            return result_t{};
        };

        using index_t =
            meta_filter_t<std::integral_constant<std::size_t, get_env_size(LhsListT{})>,
                          decltype(to_constants(make_index_sequence(meta_add_t<LhsListT, RhsListT>{}))),
                          meta_function<decltype([]<typename LhsT, typename RhsT>(LhsT, RhsT) constexpr noexcept {
                              return LhsT::value <= RhsT::value;
                          })>::template type>;

        using result_t =
            meta_filter_t<RhsListT,
                          index_t,
                          meta_function<decltype([]<typename ConstantT>(RhsListT, ConstantT) constexpr noexcept {
                              using candidates_t =
                                  meta_filter_t<meta_index_of_t<ConstantT::value - get_env_size(LhsListT{}), RhsListT>,
                                                LhsListT>;

                              return is_empty_list_v<candidates_t>;
                          })>::template type>;

        return []<typename... Ts>(type_holder<Ts...>) constexpr noexcept {
            return std::index_sequence<(Ts::value - get_env_size(LhsListT{}))...>{};
        }(result_t{});
    }

    template<typename LhsT, typename RhsT>
    [[nodiscard]] constexpr auto merge_env(LhsT&& lhs, RhsT&& rhs)
        noexcept(std::is_nothrow_constructible_v<std::remove_cvref_t<LhsT>, LhsT> &&
                 std::is_nothrow_constructible_v<std::remove_cvref_t<RhsT>, RhsT>)
    {
        using right_indices_t =
            decltype(make_unique_env_indices<to_type_holder_t<std::remove_cvref_t<LhsT>>,
                                             to_type_holder_t<std::remove_cvref_t<RhsT>>>());

        using left_indices_t =
            elements_of<std::remove_cvref_t<LhsT>>::template apply<std::index_sequence_for>;

        return [&]<std::size_t... L_INDICES, std::size_t... R_INDICES>
                   (std::index_sequence<L_INDICES...>, std::index_sequence<R_INDICES...>) {
                       return env{ std::forward_like<LhsT>(lhs.template get<L_INDICES>())...,
                                   std::forward_like<RhsT>(rhs.template get<R_INDICES>())... };
                   }(left_indices_t{}, right_indices_t{});
    }
}

#endif // !EXEC_DETAILS_MERGE_ENV_HPP
