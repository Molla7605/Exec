#ifndef EXEC_DETAILS_SCOPE_JOIN_HPP
#define EXEC_DETAILS_SCOPE_JOIN_HPP

#include "exec/completions.hpp"
#include "exec/completion_signatures.hpp"
#include "exec/env.hpp"
#include "exec/operation_state.hpp"
#include "exec/receiver.hpp"
#include "exec/scheduler.hpp"
#include "exec/sender.hpp"

#include "exec/details/basic_sender.hpp"

#include <type_traits>
#include <utility>

namespace exec::details {
    struct scope_join_t;

    template<>
    struct impls_for<scope_join_t> : default_impls {
        struct base_state {
            virtual ~base_state() = default;

            virtual void complete() noexcept = 0;

            base_state* prev{ nullptr };
        };

        template<typename ReceiverT>
        struct second_receiver {
            using receiver_concept = exec::receiver_t;

            ReceiverT& rcvr;

            void set_value() && noexcept {
                exec::set_value(std::move(rcvr));
            }

            template<typename T>
            void set_error(T&& value) && noexcept {
                exec::set_error(std::move(rcvr), std::forward<T>(value));
            }

            void set_stopped() && noexcept {
                exec::set_stopped(std::move(rcvr));
            }

            [[nodiscard]] constexpr decltype(auto) get_env() const noexcept {
                return exec::get_env(rcvr);
            }
        };


        template<typename ScopeT, typename ReceiverT>
        struct state : base_state {
            using rcvr_t = second_receiver<ReceiverT>;
            using schd_t =
                decltype(schedule(get_scheduler(exec::get_env(std::declval<ReceiverT>()))));
            using op_t = connect_result_t<schd_t, rcvr_t>;

            ScopeT* scope;
            ReceiverT& receiver;
            op_t op;

            explicit state(ScopeT* scope, ReceiverT& receiver)
                noexcept(std::is_nothrow_invocable_v<connect_t, schd_t, rcvr_t>) :
                    scope(scope),
                    receiver(receiver),
                    op(exec::connect(schedule(get_scheduler(exec::get_env(this->receiver))), rcvr_t{ this->receiver })) {}

            void complete() noexcept override {
                exec::start(op);
            }

            void complete_inline() noexcept {
                exec::set_value(std::move(receiver));
            }
        };

        static constexpr auto get_completion_signatures =
            []<typename SenderT, typename EnvT>(SenderT&&, EnvT&&) noexcept {
                if constexpr (requires{ get_scheduler(std::declval<EnvT>()); }) {
                    using schd_t = decltype(schedule(get_scheduler(std::declval<EnvT>())));
                    return completion_signatures_of_t<schd_t, EnvT>{};
                }
                else {
                    static_assert(false);
                }
            };

        static constexpr auto get_state =
            []<typename SenderT, typename ReceiverT>(SenderT&& sender, ReceiverT& receiver)
                noexcept(std::is_nothrow_constructible_v<state<std::remove_pointer_t<std::decay_t<data_of_t<SenderT>>>, ReceiverT>,
                                                         data_of_t<SenderT>,
                                                         ReceiverT>)
            {
                return state<std::remove_pointer_t<std::decay_t<data_of_t<SenderT>>>, ReceiverT>{
                    get_data(std::forward<SenderT>(sender)),
                    receiver
                };
            };

        static constexpr auto start =
            []<typename StateT>(StateT& state, auto&&) noexcept {
                if (!state.scope->try_start_join(state)) {
                    state.complete_inline();
                }
            };

    };

    struct scope_join_t {
        template<typename ScopeT>
        [[nodiscard]] constexpr auto operator()(ScopeT* scope) const noexcept {
            return details::make_sender(*this, scope);
        }
    };
    inline constexpr scope_join_t scope_join;
}

#endif // !EXEC_DETAILS_SCOPE_JOIN_HPP