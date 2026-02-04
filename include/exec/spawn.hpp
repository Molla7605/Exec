#ifndef EXEC_SPAWN_HPP
#define EXEC_SPAWN_HPP

#include "exec/env.hpp"
#include "exec/sender.hpp"
#include "exec/scope_token.hpp"

namespace exec {
    template<typename EnvT>
    struct spawn_receiver {
        void set_value() && noexcept {}
        void set_stopped() && noexcept {}
    };

    struct spawn_t {
        template<sender SenderT, scope_token TokenT, typename EnvT = empty_env>
        constexpr void operator()(SenderT&& sender, TokenT token, EnvT env = {}) const {

        }
    };
    inline constexpr spawn_t spawn{};
}

#endif // !EXEC_SPAWN_HPP