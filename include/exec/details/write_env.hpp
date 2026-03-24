#ifndef EXEC_DETAILS_WRITE_ENV_HPP
#define EXEC_DETAILS_WRITE_ENV_HPP

#include "exec/env.hpp"
#include "exec/queryable.hpp"
#include "exec/sender.hpp"

#include "exec/details/basic_sender.hpp"
#include "exec/details/join_env.hpp"

namespace exec::details {
    struct write_env_t;

    template<>
    struct impls_for<write_env_t> : default_impls {
        static constexpr auto get_completion_signatures =
            []<typename SenderT, typename EnvT>(SenderT&&, EnvT&&) noexcept {
                return completion_signatures_of_t<child_of_t<SenderT>, EnvT>{};
            };

        static constexpr auto get_env =
            []<typename StateT, typename ReceiverT>
                (auto, const StateT& state, const ReceiverT& receiver) noexcept -> decltype(auto)
            {
                return join_env(state, exec::get_env(receiver));
            };
    };

    struct write_env_t {
        template<sender SenderT, queryable EnvT>
        [[nodiscard]] constexpr auto operator()(SenderT&& sender, EnvT&& env) const noexcept {
            return details::make_sender(*this, std::forward<EnvT>(env), std::forward<SenderT>(sender));
        }
    };
    inline constexpr write_env_t write_env{};
}

#endif // !EXEC_DETAILS_WRITE_ENV_HPP