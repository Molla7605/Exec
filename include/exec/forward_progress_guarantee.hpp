#ifndef EXEC_FORWARD_PROGRESS_GUARANTEE_HPP
#define EXEC_FORWARD_PROGRESS_GUARANTEE_HPP

#include "exec/scheduler.hpp"

namespace exec {
    enum class forward_progress_guarantee {
        concurrent,
        parallel,
        weakly_parallel
    };

    struct get_forward_progress_guarantee_t {
        template<scheduler SchedulerT>
        [[nodiscard]] constexpr forward_progress_guarantee operator()(const SchedulerT& scheduler) const noexcept {
            if constexpr (requires { scheduler.query(*this); }) {
                return scheduler.query(*this);
            }

            return forward_progress_guarantee::weakly_parallel;
        }
    };
    inline constexpr get_forward_progress_guarantee_t get_forward_progress_guarantee{};
}

#endif // !EXEC_FORWARD_PROGRESS_GUARANTEE_HPP
