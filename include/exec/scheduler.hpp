#ifndef EXEC_SCHEDULER_HPP
#define EXEC_SCHEDULER_HPP

#include "exec/forwarding_query.hpp"
#include "exec/forward_progress_guarantee.hpp"
#include "exec/queryable.hpp"
#include "exec/sender.hpp"

#include "exec/details/try_query.hpp"
#include "exec/details/hide_sched.hpp"

#include <concepts>
#include <type_traits>
#include <utility>

namespace exec {
    struct scheduler_tag {};

    struct schedule_t {
        template<typename SchedulerT>
        [[nodiscard]] constexpr auto operator()(SchedulerT&& schd) const noexcept(noexcept(std::declval<SchedulerT>().schedule())) {
            return std::forward<SchedulerT>(schd).schedule();
        }
    };
    inline constexpr schedule_t schedule{};

    template<typename T>
    concept scheduler =
        std::derived_from<typename std::remove_cvref_t<T>::scheduler_concept, scheduler_tag> &&
        queryable<T> &&
        requires(T&& schd) {
            { schedule(std::forward<T>(schd)) } -> sender;
            { get_forward_progress_guarantee(schd) } -> std::same_as<forward_progress_guarantee>;
        } &&
        std::copyable<std::remove_cvref_t<T>> &&
        std::equality_comparable<std::remove_cvref_t<T>>;

    template<scheduler T>
    using schedule_result_t = decltype(schedule(std::declval<T>()));

    struct get_start_scheduler_t {
        template<typename EnvT>
        [[nodiscard]] constexpr decltype(auto) operator()(const EnvT& env) const noexcept {
            return env.query(*this);
        }

        [[nodiscard]] static consteval bool query(forwarding_query_t) noexcept {
            return true;
        }
    };
    inline constexpr get_start_scheduler_t get_start_scheduler{};

    struct get_delegation_scheduler_t {
        template<typename EnvT>
        [[nodiscard]] constexpr decltype(auto) operator()(const EnvT& env) const noexcept {
            return env.query(*this);
        }

        [[nodiscard]] static consteval bool query(forwarding_query_t) noexcept {
            return true;
        }
    };
    inline constexpr get_delegation_scheduler_t get_delegation_scheduler{};

    template<typename>
    struct get_completion_scheduler_t {
    private:
        template<typename SchedulerT, typename... EnvTs>
        [[nodiscard]] constexpr auto recurse(const SchedulerT& scheduler, EnvTs&&... envs) const noexcept {
            auto sch2 = details::try_query(scheduler,
                                           get_completion_scheduler_t<set_value_t>{},
                                           std::forward<EnvTs>(envs)...);

            if constexpr (std::is_same_v<SchedulerT, std::remove_cvref_t<decltype(sch2)>>) {
                if (sch2 != scheduler) {
                    return recurse(sch2, std::forward<EnvTs>(envs)...);
                }

                return sch2;
            }
            else {
                return recurse(sch2, std::forward<EnvTs>(envs)...);
            }
        }

    public:
        template<typename QueryableT, typename... EnvTs>
        requires details::has_query<QueryableT, get_completion_scheduler_t, EnvTs...>
        [[nodiscard]] constexpr auto operator()(const QueryableT& queryable, EnvTs&&... envs) const noexcept {
            return recurse(details::try_query(queryable, *this, std::forward<EnvTs>(envs)...), std::forward<EnvTs>(envs)...);
        }

        template<typename QueryableT, typename... EnvTs>
        requires (details::has_query<QueryableT, get_start_scheduler_t, EnvTs...> && !details::has_query<QueryableT, get_completion_scheduler_t, EnvTs...>)
        [[nodiscard]] constexpr auto operator()(const QueryableT& queryable, EnvTs&&... envs) const noexcept {
            return recurse(details::try_query(queryable, get_start_scheduler, std::forward<EnvTs>(envs)...), std::forward<EnvTs>(envs)...);
        }

        template<scheduler SchedulerT, typename... EnvTs>
        requires (!details::has_query<SchedulerT, get_completion_scheduler_t, EnvTs...>)
        [[nodiscard]] constexpr auto operator()(const SchedulerT& scheduler, EnvTs&&...) const noexcept {
            return auto(scheduler);
        }

        [[nodiscard]] static consteval bool query(forwarding_query_t) noexcept {
            return true;
        }
    };
    template<typename CompletionT>
    inline constexpr get_completion_scheduler_t<CompletionT> get_completion_scheduler{};

    struct get_scheduler_t {
        template<typename EnvT>
        [[nodiscard]] constexpr decltype(auto) operator()(const EnvT& env) const noexcept {
            return get_completion_scheduler<exec::set_value_t>(env.query(*this), details::hide_sched(env));
        }

        [[nodiscard]] static consteval bool query(forwarding_query_t) noexcept {
            return true;
        }
    };
    inline constexpr get_scheduler_t get_scheduler{};
}

#endif // !EXEC_SCHEDULER_HPP