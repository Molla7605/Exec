#ifndef EXEC_CONTINUES_ON_HPP
#define EXEC_CONTINUES_ON_HPP

#include "exec/scheduler.hpp"
#include "exec/schedule_from.hpp"
#include "exec/sender.hpp"
#include "exec/sender_adapter_closure.hpp"

#include "exec/details/basic_closure.hpp"

#include <utility>

namespace exec {
    struct continues_on_t {
        template<sender SenderT, scheduler SchedulerT>
        constexpr auto operator()(SenderT&& input, SchedulerT&& scheduler) const {
            return schedule_from(std::forward<SchedulerT>(scheduler), std::forward<SenderT>(input));
        }

        template<scheduler SchedulerT>
        [[nodiscard]] constexpr auto operator()(SchedulerT&& scheduler) const noexcept {
            return details::basic_closure{
                sender_adapter_closure<continues_on_t>{},
                details::product_type{ std::forward<SchedulerT>(scheduler) }
            };
        }
    };
    inline constexpr continues_on_t continues_on{};
}

#endif // !EXEC_CONTINUES_ON_HPP