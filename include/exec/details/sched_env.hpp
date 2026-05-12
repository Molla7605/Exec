#ifndef EXEC_DETAILS_SCHED_ENV_HPP
#define EXEC_DETAILS_SCHED_ENV_HPP

#include "exec/scheduler.hpp"

#include <type_traits>

namespace exec::details {
    template<typename SchedulerT>
    struct scheduler_attributes {
        SchedulerT scheduler;

        [[nodiscard]] constexpr auto query(get_start_scheduler_t) const
            noexcept(std::is_nothrow_copy_constructible_v<SchedulerT>)
        {
            return scheduler;
        }
    };

    template<scheduler SchedulerT>
    scheduler_attributes(SchedulerT) -> scheduler_attributes<std::decay_t<SchedulerT>>;

    template<scheduler SchedulerT>
    [[nodiscard]] constexpr auto sched_env(SchedulerT schd)
        noexcept(std::is_nothrow_constructible_v<scheduler_attributes<SchedulerT>, SchedulerT>)
    {
        return scheduler_attributes{ schd };
    }
}

#endif // !EXEC_DETAILS_SCHED_ENV_HPP