#ifndef EXEC_ENV_HPP
#define EXEC_ENV_HPP

#include "exec/queryable.hpp"

#include "exec/details/env_traits.hpp"
#include "exec/details/meta_filter.hpp"
#include "exec/details/product_type.hpp"
#include "exec/details/unique_template.hpp"

#include <type_traits>
#include <utility>

namespace exec {
    template<typename TagT, typename ValueT>
    struct prop {
        TagT tag;
        ValueT value;

        [[nodiscard]] constexpr const ValueT& query(TagT) const noexcept {
            return value;
        }
    };

    template<typename TagT, typename ValueT>
    prop(TagT, ValueT) -> prop<TagT, std::decay_t<ValueT>>;

    template<typename TagT, typename ValueT>
    prop(TagT, std::reference_wrapper<ValueT>) -> prop<TagT, ValueT&>;

    template<typename TagT, typename ValueT>
    prop(TagT, std::reference_wrapper<const ValueT>) -> prop<TagT, const ValueT&>;

    template<queryable... PropTs>
    class env : public details::product_type<PropTs...> {
        template<typename TagT, typename IndexT>
        struct has_query_filter {
            static constexpr bool value = details::has_query<details::meta_index_t<IndexT::value, PropTs...>, TagT>;
        };

        template<typename...>
        struct make_index_constant_sequence {};

        template<std::size_t... INDICES>
        struct make_index_constant_sequence<std::index_sequence<INDICES...>> {
            using type = details::type_holder<std::integral_constant<std::size_t, INDICES>...>;
        };

    public:
        template<typename TagT, typename... ArgTs>
        requires (details::has_query<PropTs, TagT> || ...)
        [[nodiscard]] constexpr decltype(auto) query(TagT, ArgTs&&... args) const noexcept {
            using candidates_t =
                details::meta_filter_t<TagT,
                                       typename make_index_constant_sequence<std::index_sequence_for<PropTs...>>::type,
                                       has_query_filter>;

            using index_t = details::meta_index_of_t<0, candidates_t>;

            return this->template get<index_t::value>().query(TagT{}, std::forward<ArgTs>(args)...);
        }

    };

    template<>
    class env<> {};

    template<queryable... PropTs>
    env(PropTs...) -> env<std::decay_t<PropTs>...>;

    using empty_env = env<>;

    struct get_env_t {
        template<queryable QueryableT>
        [[nodiscard]] constexpr decltype(auto) operator()(const QueryableT& queryable) const noexcept {
            if constexpr (requires { queryable.get_env(); }) {
                return queryable.get_env();
            }
            else {
                return empty_env{};
            }
        }
    };
    inline constexpr get_env_t get_env{};

    template<typename T>
    using env_of_t = decltype(get_env(std::declval<T>()));
}

#endif // !EXEC_ENV_HPP