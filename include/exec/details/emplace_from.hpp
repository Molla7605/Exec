#ifndef EXEC_DETAILS_EMPLACE_FROM_HPP
#define EXEC_DETAILS_EMPLACE_FROM_HPP

#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>

namespace exec::details {
    template<std::invocable InvocableT>
    struct emplace_from {
        using result_t = std::invoke_result_t<InvocableT>;

        InvocableT invocable;

        [[nodiscard]] constexpr operator result_t() && noexcept(std::is_nothrow_invocable_v<InvocableT>) {
            return std::invoke(std::move(invocable));
        }

        [[nodiscard]] constexpr result_t operator()() && noexcept(std::is_nothrow_invocable_v<InvocableT>) {
            return std::invoke(std::move(invocable));
        }
    };
}

#endif // !EXEC_DETAILS_EMPLACE_FROM_HPP