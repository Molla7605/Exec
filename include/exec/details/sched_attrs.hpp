#ifndef EXEC_DETAILS_SCHED_ATTRS_HPP
#define EXEC_DETAILS_SCHED_ATTRS_HPP

#include "exec/scheduler.hpp"

#include <type_traits>

namespace exec::details {
    template<typename SchedulerT>
    struct scheduler_attributes {
        SchedulerT scheduler;

        template<typename CompletionT>
        requires std::invocable<get_completion_scheduler_t<CompletionT>, env_of_t<schedule_result_t<SchedulerT>>>
        [[nodiscard]] constexpr auto query(get_completion_scheduler_t<CompletionT>) const
            noexcept(std::is_nothrow_copy_constructible_v<SchedulerT>)
        {
            return scheduler;
        }
    };

    template<scheduler SchedulerT>
    scheduler_attributes(SchedulerT) -> scheduler_attributes<std::decay_t<SchedulerT>>;

    template<scheduler SchedulerT>
    [[nodiscard]] constexpr auto sched_attrs(SchedulerT schd)
        noexcept(std::is_nothrow_constructible_v<scheduler_attributes<std::decay_t<SchedulerT>>>)
    {
        return scheduler_attributes{ schd };
    }
}

#endif // !EXEC_DETAILS_SCHED_ATTRS_HPP