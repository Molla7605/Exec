#ifndef EXEC_DETAILS_ASSOCIATION_HPP
#define EXEC_DETAILS_ASSOCIATION_HPP

#include "exec/scope_token.hpp"

#include <type_traits>

namespace exec::details {
    template<typename ScopeT>
    struct association_t {
        ScopeT* scope{ nullptr };

        association_t() = default;

        association_t(ScopeT* scope) noexcept : scope(scope) {}

        association_t(const association_t& scope) noexcept = delete;

        association_t& operator=(const association_t& scope) noexcept = delete;

        association_t(association_t&& other) noexcept : scope(std::exchange(other.scope, nullptr)) {}

        association_t& operator=(association_t&& other) noexcept {
            scope = std::exchange(other.scope, nullptr);
            return *this;
        }

        ~association_t() noexcept {
            if (scope != nullptr) {
                scope->disassociate();
            }
        }

        [[nodiscard]] association_t try_associate() const {
            if (scope != nullptr) {
                return scope->try_associate();
            }

            return {};
        }

        [[nodiscard]] constexpr operator bool() const noexcept {
            return scope != nullptr;
        }

    };

    template<scope_token T>
    using association_of_t = std::remove_cvref_t<decltype(std::declval<std::remove_cvref_t<T>&>().try_associate())>;
}

#endif // !EXEC_DETAILS_ASSOCIATION_HPP