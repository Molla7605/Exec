#ifndef EXEC_STARTS_ON_HPP
#define EXEC_STARTS_ON_HPP

#include "exec/let.hpp"
#include "exec/sender.hpp"
#include "exec/scheduler.hpp"

#include <utility>

namespace exec {
    struct starts_on_t {
        [[nodiscard]] constexpr auto operator()(scheduler auto&& scheduler, sender auto&& input) const {
            return let_value(schedule(std::forward<decltype(scheduler)>(scheduler)),
                             [sndr = std::forward<decltype(input)>(input)]() mutable { return std::move(sndr); });
        }
    };
    inline constexpr starts_on_t starts_on{};
}

#endif // !EXEC_STARTS_ON_HPP