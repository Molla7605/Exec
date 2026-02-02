#ifndef EXEC_QUERY_HPP
#define EXEC_QUERY_HPP

#include <concepts>
#include <type_traits>

namespace exec {
    template<typename Type>
    concept queryable = std::destructible<Type>;

    struct forwarding_query_t {
        template<typename QueryT>
        [[nodiscard]] consteval bool operator()(const QueryT& query) const noexcept {
            if constexpr (requires { std::declval<QueryT>().query(std::declval<forwarding_query_t>()); }) {
                return query.query(*this);
            }

            return std::derived_from<QueryT, forwarding_query_t>;
        }
    };
    inline constexpr forwarding_query_t forwarding_query{};
}

#endif // !EXEC_QUERY_HPP