#ifndef EXEC_UNSTOPPABLE_HPP
#define EXEC_UNSTOPPABLE_HPP

#include "exec/env.hpp"
#include "exec/sender.hpp"
#include "exec/stop_token.hpp"

#include "exec/details/write_env.hpp"

namespace exec {
    struct unstoppable_t {
        template<sender SenderT>
        [[nodiscard]] constexpr auto operator()(SenderT&& sender) const {
            return write_env(sender, prop(get_stop_token, never_stop_token{}));
        }
    };
    inline constexpr unstoppable_t unstoppable{};
}

#endif //EXEC_UNSTOPPABLE_HPP
