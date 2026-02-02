#ifndef EXEC_CONTINUES_ON_HPP
#define EXEC_CONTINUES_ON_HPP

#include "exec/pipe_adapter.hpp"
#include "exec/scheduler.hpp"
#include "exec/schedule_from.hpp"
#include "exec/sender.hpp"

#include <tuple>
#include <utility>

namespace exec {
    struct continues_on_t {
        constexpr auto operator()(sender auto&& input, scheduler auto&& scheduler) const {
            return schedule_from(std::forward<decltype(scheduler)>(scheduler), std::forward<decltype(input)>(input));
        }

        constexpr auto operator()(scheduler auto&& scheduler) const {
            return pipe_adapter {
                [this]<typename input_t, typename scheduler_t>(input_t&& input, scheduler_t&& schd) constexpr {
                    return this->operator()(std::forward<input_t>(input), std::forward<scheduler_t>(schd));
                },
                std::make_tuple(std::forward<decltype(scheduler)>(scheduler))
            };
        }
    };
    inline constexpr continues_on_t continues_on{};
}

#endif // !EXEC_CONTINUES_ON_HPP