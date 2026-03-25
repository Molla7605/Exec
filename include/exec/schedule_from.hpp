#ifndef EXEC_SCHEDULE_FROM_HPP
#define EXEC_SCHEDULE_FROM_HPP

#include "exec/completions.hpp"
#include "exec/completion_signatures.hpp"
#include "exec/env.hpp"
#include "exec/operation_state.hpp"
#include "exec/receiver.hpp"
#include "exec/scheduler.hpp"
#include "exec/sender.hpp"
#include "exec/transform_completion_signatures.hpp"

#include "exec/details/basic_sender.hpp"
#include "exec/details/conditional_meta_apply.hpp"
#include "exec/details/decayed_tuple.hpp"
#include "exec/details/dummy_receiver.hpp"
#include "exec/details/forward_env.hpp"
#include "exec/details/is_nothrow_signatures.hpp"
#include "exec/details/join_env.hpp"
#include "exec/details/meta_bind.hpp"
#include "exec/details/meta_index.hpp"
#include "exec/details/meta_merge.hpp"
#include "exec/details/sched_attrs.hpp"
#include "exec/details/signature_info.hpp"
#include "exec/details/type_holder.hpp"
#include "exec/details/type_list.hpp"

#include <concepts>
#include <exception>
#include <print>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace exec {
    struct schedule_from_t;

    template<>
    struct details::impls_for<schedule_from_t> : default_impls {
        template<typename SigT>
        using as_tuple_t =
            signature_args_of<completion_signatures<SigT>>::template apply<
                meta_bind_front<decayed_tuple,
                                completion_tag_of_t<completion_signatures<SigT>>>::template type>;

        template<typename... SigTs>
        using as_variant_t = std::variant<std::monostate, as_tuple_t<SigTs>...>;

        template<typename... ArgTs>
        struct nothrow_movable {
            static constexpr bool value =
                []() consteval {
                    using tuple_t =
                        std::tuple<std::bool_constant<std::is_nothrow_constructible_v<std::decay_t<ArgTs>, ArgTs>>...>;

                    return std::apply([]<typename... Ts>(Ts&&... values) noexcept {
                        return (values && ... && true);
                    }, tuple_t{});
                }();
        };

        template<typename StateT, typename ReceiverT>
        struct second_receiver {
            using receiver_concept = exec::receiver_t;

            StateT* state;

            void set_value() && noexcept {
                std::visit(
                    [this]<typename TupleT>(TupleT& tuple) noexcept {
                        if constexpr (!std::same_as<std::monostate, TupleT>) {
                            std::apply(
                                [this]<typename TagT, typename... Ts>(TagT& tag, Ts&... values) noexcept {
                                    tag(std::move(state->receiver), std::move(values)...);
                                },
                                tuple
                            );
                        }
                    },
                    state->results
                );
            }

            template<typename T>
            void set_error(T&& value) && noexcept {
                exec::set_error(std::move(state->receiver), std::forward<T>(value));
            }

            void set_stopped() && noexcept {
                exec::set_stopped(std::move(state->receiver));
            }

            [[nodiscard]] constexpr forward_env_of_t<ReceiverT> get_env() const noexcept {
                return forward_env(exec::get_env(state->receiver));
            }
        };

        static constexpr auto get_attrs =
            []<typename SchedulerT, typename ChildT>(const SchedulerT& scheduler, const ChildT& child) noexcept {
                return join_env(sched_attrs(scheduler), forward_env(exec::get_env(child)));
            };

        static constexpr auto get_completion_signatures =
            []<typename SenderT, typename EnvT>(SenderT&&, EnvT&&) noexcept {
                using child_sender_t = decltype(std::declval<SenderT>().template get<2>());
                using schedule_t = schedule_result_t<meta_index_of_t<1, std::decay_t<SenderT>>>;
                using child_completion_signatures_t =
                    conditional_meta_apply_t<
                        sends_stopped<schedule_t, EnvT>,
                        meta_bind_front<meta_merge_t, completion_signatures_of_t<child_sender_t, EnvT>>::template type,
                        type_holder<completion_signatures<set_stopped_t()>>,
                        type_holder<>
                    >;

                constexpr bool nothrow = is_nothrow_signatures<nothrow_movable, child_completion_signatures_t>;

                if constexpr (nothrow) {
                    return child_completion_signatures_t{};
                }
                else {
                    return meta_merge_t<child_completion_signatures_t,
                                        completion_signatures<set_error_t(std::exception_ptr)>>{};
                }
            };

        static constexpr auto get_state =
            []<typename SenderT, typename ReceiverT>(SenderT&& sender, ReceiverT& receiver)
                noexcept(std::is_nothrow_invocable_v<connect_t,
                                                     schedule_result_t<meta_index_of_t<1, std::decay_t<SenderT>>>,
                                                     dummy_receiver<env_of_t<ReceiverT>>>)
                requires sender_in<child_of_t<SenderT>, env_of_t<ReceiverT>>
            {
                struct state {
                    using impls = impls_for<schedule_from_t>;
                    using receiver_t = impls::second_receiver<state, ReceiverT>;
                    using scheduler_t = meta_index_of_t<1, std::decay_t<SenderT>>;
                    using schedule_t = schedule_result_t<scheduler_t>;
                    using op_t = connect_result_t<schedule_t, receiver_t>;

                    using child_completion_signatures_t = completion_signatures_of_t<SenderT, env_of_t<ReceiverT>>;

                    using variant_t =
                        elements_of<child_completion_signatures_t>::template apply<as_variant_t>;

                    ReceiverT& receiver;
                    variant_t results;
                    op_t op;

                    explicit state(scheduler_t schd, ReceiverT& receiver)
                        noexcept(noexcept(exec::connect(exec::schedule(schd), receiver_t{ nullptr }))) :
                            receiver(receiver),
                            op(exec::connect(exec::schedule(schd), receiver_t{ this })) {}
                };

                return state{ get_data(std::forward<SenderT>(sender)), receiver };
            };

        static constexpr auto complete =
            []<typename StateT, typename ReceiverT, typename TagT, typename... ArgTs>
                (auto, StateT& state, ReceiverT& receiver, TagT, ArgTs&&... args) noexcept -> void
            {
                using result_t = decayed_tuple<TagT, ArgTs...>;
                constexpr bool nothrow = std::is_nothrow_constructible_v<result_t, TagT, ArgTs...>;

                try {
                    state.results.template emplace<result_t>(TagT{}, std::forward<ArgTs>(args)...);
                }
                catch (...) {
                    if constexpr (!nothrow) {
                        exec::set_error(std::move(receiver), std::current_exception());
                    }
                }

                if (state.results.valueless_by_exception()) {
                    return;
                }
                if (state.results.index() == 0) {
                    return;
                }

                exec::start(state.op);
            };
    };

    struct schedule_from_t {
        template<scheduler SchedulerT, sender SenderT>
        [[nodiscard]] constexpr auto operator()(SchedulerT&& scheduler, SenderT&& input) const {
            return details::make_sender(*this, std::forward<SchedulerT>(scheduler), std::forward<SenderT>(input));
        }
    };
    inline constexpr schedule_from_t schedule_from{};
}

#endif // !EXEC_SCHEDULE_FROM_HPP