#ifndef EXEC_SCHEDULE_FROM_HPP
#define EXEC_SCHEDULE_FROM_HPP

#include "exec/completion_signatures.hpp"
#include "exec/env.hpp"
#include "exec/scheduler.hpp"
#include "exec/sender.hpp"
#include "exec/receiver.hpp"
#include "exec/operation_state.hpp"
#include "exec/transform_completion_signatures.hpp"

#include "exec/details/decayed_tuple.hpp"
#include "exec/details/as_tuple.hpp"
#include "exec/details/type_list.hpp"
#include "exec/details/unique_template.hpp"


#include <variant>
#include <tuple>
#include <type_traits>
#include <utility>

namespace exec {
    template<typename OpT>
    struct schedule_from_receiver1 {
        using receiver_concept = receiver_t;

        OpT* op;

        template<typename... Ts>
        void set_value(Ts&&... values) && noexcept {
            op->template transfer_context<set_value_t>(std::forward<decltype(values)>(values)...);
        }

        template<typename T>
        void set_error(T&& value) && noexcept {
            op->template transfer_context<set_error_t>(std::forward<decltype(value)>(value));
        }

        void set_stopped() && noexcept {
            op->template transfer_context<set_stopped_t>();
        }

        [[nodiscard]] constexpr forward_env_of_t<typename OpT::receiver_t> query(get_env_t) const noexcept {
            return forward_env(get_env(op->receiver));
        }
    };

    template<typename OpT>
    struct schedule_from_receiver2 {
        using receiver_concept = receiver_t;

        OpT* op;

        void set_value() && noexcept {
            op->complete();
        }

        template<typename T>
        void set_error(T&& value) && noexcept {
            op->template passthrough<set_error_t>(std::forward<decltype(value)>(value));
        }

        void set_stopped() && noexcept {
            op->template passthrough<set_stopped_t>();
        }

        [[nodiscard]] constexpr forward_env_of_t<typename OpT::receiver_t> query(get_env_t) const noexcept {
            return forward_env(get_env(op->receiver));
        }
    };

    template<typename SchedulerT, typename SenderT, typename ReceiverT>
    struct schedule_from_operation_state {
        using operation_state_concept = operation_state_t;

        using receiver_t = ReceiverT;

        using first_op_t = connect_result_t<SenderT,
                                            details::indirect_meta_apply_t<schedule_from_receiver1, schedule_from_operation_state>>;
        using second_op_t = connect_result_t<details::schedule_result_t<SchedulerT>,
                                             details::indirect_meta_apply_t<schedule_from_receiver2, schedule_from_operation_state>>;
        using state_t = std::variant<SenderT, first_op_t, second_op_t>;

        template<typename... Ts>
        using arg_variant_t = std::variant<std::monostate, details::as_tuple_t<Ts>...>;

        using args_t =
            details::elements_of<completion_signatures_of_t<SenderT, env_of_t<ReceiverT>>>::template apply<arg_variant_t>;

        SchedulerT scheduler;
        ReceiverT receiver;
        state_t state;
        args_t args;

        void complete() & noexcept {
            std::visit(
                [&]<typename TupleT>(TupleT& tuple) {
                    if constexpr (!std::is_same_v<std::remove_cvref_t<TupleT>, std::monostate>) {
                        std::apply(
                            [&]<typename TagT, typename... Ts>(TagT& tag, Ts&... values) {
                                tag(std::move(receiver), std::move(values)...);
                            },
                            tuple
                        );
                    }
                },
                args
            );
        }

        template<typename TagT>
        void transfer_context(auto&&... values) & noexcept {
            using result_t = details::decayed_tuple<TagT, decltype(values)...>;

            if constexpr (details::has_completion_tag_v<TagT, completion_signatures_of_t<SenderT, env_of_t<ReceiverT>>>) {
                try {
                    args.template emplace<result_t>(TagT{}, std::forward<decltype(values)>(values)...);
                }
                catch (...) {
                    exec::set_error(std::move(receiver), std::current_exception());
                }
            }

            if (args.valueless_by_exception()) {
                return;
            }
            if (args.index() == 0) {
                return;
            }

            exec::start(state.template emplace<second_op_t>(exec::connect(exec::schedule(scheduler), schedule_from_receiver2{ this })));
        }

        template<typename TagT>
        void passthrough(auto&&... values) & noexcept {
            TagT{}(std::move(receiver), std::forward<decltype(values)>(values)...);
        }

        void start() & noexcept {
            auto& op = state.template emplace<first_op_t>(exec::connect(std::get<SenderT>(state), schedule_from_receiver1{ this }));

            exec::start(op);
        }
    };

    template<typename SchedulerT, typename SenderT>
    struct schedule_from_sender {
        using sender_concept = exec::sender_t;

        SchedulerT scheduler;
        SenderT sender;

        template<typename EnvT>
        [[nodiscard]] constexpr auto get_completion_signatures(const EnvT&) const noexcept {
            return completion_signatures_of_t<SenderT, EnvT>{};
        }

        template<typename Self, receiver ReceiverT>
        [[nodiscard]] constexpr auto connect(this Self&& self, ReceiverT&& receiver) {
            return schedule_from_operation_state<SchedulerT, SenderT, ReceiverT> {
                self.scheduler,
                std::forward<ReceiverT>(receiver),
                { std::forward_like<Self>(self.sender) },
                {}
            };
        }
    };

    struct schedule_from_t {
        template<scheduler SchedulerT, sender SenderT>
        [[nodiscard]] constexpr auto operator()(SchedulerT&& scheduler, SenderT&& input) const {
            return schedule_from_sender<std::decay_t<SchedulerT>, std::decay_t<SenderT>> {
                std::forward<SchedulerT>(scheduler), std::forward<SenderT>(input)
            };
        }
    };
    inline constexpr schedule_from_t schedule_from{};
}

#endif // !EXEC_SCHEDULE_FROM_HPP