#ifndef EXEC_SYNC_WAIT_HPP
#define EXEC_SYNC_WAIT_HPP

#include "exec/env.hpp"
#include "exec/sender.hpp"

#include "exec/details/sync_wait_state.hpp"

#include <exception>
#include <optional>
#include <type_traits>
#include <utility>

namespace exec {
    template<typename SenderT>
    struct sync_wait_receiver {
        using receiver_concept = receiver_t;

        details::sync_wait_state<SenderT>* state;

        template<typename... Ts>
        void set_value(Ts&&... values) && noexcept {
            try {
                state->result.emplace(std::forward<Ts>(values)...);
            }
            catch (...) {
                state->error.emplace(std::current_exception());
            }

            state->loop.finish();
        }

        template<typename T>
        void set_error(T&& value) && noexcept {
            try {
                state->error.emplace(std::forward<T>(value));
            }
            catch (...) {
                state->error.emplace(std::current_exception());
            }

            state->loop.finish();
        }

        void set_stopped() && noexcept {
            state->result = std::nullopt;

            state->loop.finish();
        }

        [[nodiscard]] constexpr details::sync_wait_env query(get_env_t) const noexcept {
            return { &state->loop };
        }
    };

    struct sync_wait_t {
        template<sender SenderT>
        requires sender_in<std::remove_cvref_t<SenderT>, details::sync_wait_env>
        [[nodiscard]] constexpr auto operator()(SenderT&& input) const {
            details::sync_wait_state<std::decay_t<SenderT>> state{};

            auto op = exec::connect(std::forward<SenderT>(input), sync_wait_receiver<std::decay_t<SenderT>>{ &state });
            exec::start(op);

            state.loop.run();
            if (state.error.has_value()) {
                std::visit(details::sync_wait_error_handler, *state.error);
            }

            return std::move(state.result);
        }
    };
    inline constexpr sync_wait_t sync_wait{};
}

#endif // !EXEC_SYNC_WAIT_HPP