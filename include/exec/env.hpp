#ifndef EXEC_ENV_HPP
#define EXEC_ENV_HPP

#include "exec/queryable.hpp"

#include "exec/details/product_type.hpp"
#include "exec/details/unique_template.hpp"
#include "exec/details/meta_filter.hpp"

#include <type_traits>
#include <utility>

namespace exec {
    template<typename EnvT, typename TagT>
    concept has_query =
        requires(const EnvT& env) {
            env.query(TagT{});
        };

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

    template<queryable... PropTs>
    requires std::is_same_v<details::meta_unique_t<details::type_holder<PropTs...>>, details::type_holder<PropTs...>>
    struct env : private details::product_type<PropTs...> {
    private:
        template<typename IndexT, typename TagT>
        struct has_query_filter {
            static constexpr bool value = has_query<details::meta_index_t<IndexT::value, PropTs...>, TagT>;
        };

        template<typename...>
        struct make_index_constant_sequence {};

        template<std::size_t... INDICES>
        struct make_index_constant_sequence<std::index_sequence<INDICES...>> {
            using type = details::type_holder<std::index_sequence<INDICES>...>;
        };

    public:
        template<typename TagT>
        requires (has_query<PropTs, TagT> || ...)
        [[nodiscard]] constexpr decltype(auto) query(TagT) const noexcept {
            using candidates_t =
                details::meta_filter_t<TagT,
                                       typename make_index_constant_sequence<std::index_sequence_for<PropTs...>>::type,
                                       has_query_filter>;

            return this->template get<details::meta_index_of_t<0, candidates_t>::value>().query(TagT{});
        }

    };

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

    template<typename QueryableT>
    using env_of_t = std::invoke_result_t<get_env_t, QueryableT>;
}

#endif // !EXEC_ENV_HPP