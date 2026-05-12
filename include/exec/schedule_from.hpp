#ifndef EXEC_SCHEDULE_FROM_HPP
#define EXEC_SCHEDULE_FROM_HPP

#include "exec/continues_on.hpp"
#include "exec/scheduler.hpp"
#include "exec/sender.hpp"

#include <utility>

namespace exec {
    struct schedule_from_t {
        template<scheduler SchedulerT, sender SenderT>
        [[nodiscard]] constexpr auto operator()(SchedulerT&& scheduler, SenderT&& input) const {
            return continues_on(std::forward<SenderT>(input), std::forward<SchedulerT>(scheduler));
        }
    };
    inline constexpr schedule_from_t schedule_from{};
}

#endif // !EXEC_SCHEDULE_FROM_HPP