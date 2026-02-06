#ifndef EXEC_ALLOCATOR_HPP
#define EXEC_ALLOCATOR_HPP

#include "exec/query.hpp"

#include <concepts>
#include <cstddef>
#include <type_traits>

namespace exec {
    template<typename T>
    concept allocator =
        requires(T alloc, std::size_t size) {
            { *alloc.allocate(size) } -> std::same_as<typename T::value_type&>;
            { alloc.deallocate(alloc.allocate(size), size) };
        } &&
        std::copy_constructible<T> &&
        std::equality_comparable<T>;

    struct get_allocator_t {
        template<typename EnvT>
        [[nodiscard]] constexpr allocator decltype(auto) operator()(const EnvT& env) const noexcept {
            return env.query(*this);
        }

        [[nodiscard]] static consteval bool query(forwarding_query_t) noexcept {
            return true;
        }
    };
    inline constexpr get_allocator_t get_allocator{};
}

#endif // !EXEC_ALLOCATOR_HPP