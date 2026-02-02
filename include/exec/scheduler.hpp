#ifndef EXEC_SCHEDULER_HPP
#define EXEC_SCHEDULER_HPP

#include "exec/completion_signatures.hpp"
#include "exec/env.hpp"
#include "exec/sender.hpp"

#include <concepts>
#include <type_traits>
#include <utility>

namespace exec {
    struct scheduler_t {};

    struct schedule_t {
        [[nodiscard]] constexpr sender auto operator()(auto&& schd) const {
            return std::forward<decltype(schd)>(schd).schedule();
        }
    };
    inline constexpr schedule_t schedule{};

    struct get_scheduler_t {
        template<typename EnvT>
        [[nodiscard]] constexpr decltype(auto) operator()(const EnvT& env) const noexcept {
            return env.query(*this);
        }
    };
    inline constexpr get_scheduler_t get_scheduler{};

    struct get_delegation_scheduler_t {
        template<typename EnvT>
        [[nodiscard]] constexpr decltype(auto) operator()(const EnvT& env) const noexcept {
            return env.query(*this);
        }
    };
    inline constexpr get_delegation_scheduler_t get_delegation_scheduler{};

    template<typename TagT>
    struct get_completion_scheduler_t {
        template<typename EnvT>
        [[nodiscard]] constexpr decltype(auto) operator()(const EnvT& env) const noexcept {
            return env.query(*this);
        }
    };
    template<typename TagT>
    inline constexpr get_completion_scheduler_t<TagT> get_completion_scheduler{};

    template<typename T>
    concept scheduler =
        std::derived_from<typename std::remove_cvref_t<T>::scheduler_concept, scheduler_t> &&
        queryable<T> &&
        requires(T&& schd) {
            { schedule(std::forward<T>(schd)) } -> sender;
            { auto(get_completion_scheduler<set_value_t>(get_env(schedule(std::forward<T>(schd))))) } ->
                std::same_as<std::remove_cvref_t<T>>;
        } &&
        std::is_copy_constructible_v<T> &&
        std::equality_comparable<T>;

    namespace details {
        template<scheduler T>
        using schedule_result_t = decltype(schedule(std::declval<T>()));
    }
}

#endif // !EXEC_SCHEDULER_HPP