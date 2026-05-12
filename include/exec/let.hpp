#ifndef EXEC_LET_HPP
#define EXEC_LET_HPP

#include "exec/completions.hpp"
#include "exec/completion_signatures.hpp"
#include "exec/env.hpp"
#include "exec/operation_state.hpp"
#include "exec/receiver.hpp"
#include "exec/scheduler.hpp"
#include "exec/sender.hpp"
#include "exec/sender_adapter_closure.hpp"

#include "exec/details/basic_closure.hpp"
#include "exec/details/basic_sender.hpp"
#include "exec/details/dummy_receiver.hpp"
#include "exec/details/emplace_from.hpp"
#include "exec/details/forward_env.hpp"
#include "exec/details/gather_signatures.hpp"
#include "exec/details/is_nothrow_signatures.hpp"
#include "exec/details/join_env.hpp"
#include "exec/details/meta_add.hpp"
#include "exec/details/meta_bind.hpp"
#include "exec/details/meta_index.hpp"
#include "exec/details/meta_merge.hpp"
#include "exec/details/meta_not.hpp"
#include "exec/details/unique_template.hpp"

#include <exception>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace exec {
    template<typename CompletionT>
    struct let_tag_t;

    template<typename CompletionT>
    struct details::impls_for<let_tag_t<CompletionT>> : default_impls {
        template<typename ReceiverT, typename EnvT>
        struct second_receiver {
            using receiver_concept = exec::receiver_tag;

            ReceiverT& receiver;
            EnvT env;

            template<typename... Ts>
            void set_value(Ts&&... values) && noexcept {
                exec::set_value(std::move(receiver), std::forward<Ts>(values)...);
            }

            template<typename T>
            void set_error(T&& value) && noexcept {
                exec::set_error(std::move(receiver), std::forward<T>(value));
            }

            void set_stopped() && noexcept {
                exec::set_stopped(std::move(receiver));
            }

            [[nodiscard]] constexpr auto get_env() const noexcept {
                return join_env(env, forward_env(exec::get_env(receiver)));
            }
        };

        template<typename InvocableT, typename ReceiverT, typename... ArgTs>
        using as_op_t =
            connect_result_t<std::invoke_result_t<InvocableT, std::decay_t<ArgTs>&...>, ReceiverT>;

        template<typename InvocableT, valid_type_holder EnvListT, typename... ArgTs>
        using signatures_t =
            elements_of<EnvListT>::template apply<
                meta_bind_front<completion_signatures_of_t,
                                std::invoke_result_t<InvocableT, std::decay_t<ArgTs>&...>>::template type>;

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

        template<typename InvocableT, typename ReceiverT, typename... ArgTs>
        struct nothrow_connectable {
            static constexpr bool value =
                std::is_nothrow_invocable_v<connect_t, std::invoke_result_t<InvocableT, ArgTs...>, ReceiverT>;
        };

        template<typename SenderT, typename EnvT>
        [[nodiscard]] static constexpr auto make_env(const SenderT& sender, const EnvT& env) {
            if constexpr (has_query<EnvT, get_completion_scheduler_t<CompletionT>, forward_env_of_t<EnvT>>) {
                return sched_env(get_completion_scheduler<CompletionT>(exec::get_env(sender), forward_env(env)));
            }
            else {
                return empty_env{};
            }
        }

        template<typename SenderT, typename InvocableT, typename ReceiverT, typename ArgsVariantT, typename OpsVariantT>
        struct let_state {
            struct inner_receiver {
                let_state& state;
                ReceiverT& receiver;

                using receiver_concept = exec::receiver_tag;

                template<typename... Ts>
                constexpr void set_value(Ts&&... values) && noexcept {
                    state.impl(receiver, exec::set_value, std::forward<Ts>(values)...);
                }

                template<typename T>
                constexpr void set_error(T&& value) && noexcept {
                    state.impl(receiver, exec::set_error, std::forward<T>(value));
                }

                constexpr void set_stopped() && noexcept {
                    state.impl(receiver, exec::set_stopped);
                }

                constexpr env_of_t<const ReceiverT&> get_env() const noexcept {
                    return exec::get_env(receiver);
                }
            };

            using env_t = decltype(make_env(std::declval<SenderT>(), exec::get_env(std::declval<ReceiverT>())));
            using op_t = connect_result_t<SenderT, inner_receiver>;

            InvocableT invocable;
            env_t env;
            ArgsVariantT args_variant;
            meta_append_back_t<OpsVariantT, op_t> ops_variant;

            template<typename TagT, typename... Ts>
            constexpr void impl(ReceiverT receiver, TagT tag, Ts&&... args) noexcept {
                if constexpr (std::is_same_v<TagT, CompletionT>) {
                    using args_t = decayed_tuple<Ts...>;
                    using receiver_t = second_receiver<ReceiverT, env_t>;
                    using sender_t = std::invoke_result_t<InvocableT, std::decay_t<Ts>&...>;

                    try {
                        auto& tuple = args_variant.template emplace<args_t>(std::forward<Ts>(args)...);
                        ops_variant.template emplace<std::monostate>();

                        auto&& sender = std::apply(std::move(invocable), tuple);

                        using op2_t = connect_result_t<sender_t, receiver_t>;

                        auto make_op2 = [&]() {
                            return exec::connect(std::forward<sender_t>(sender),
                                                 second_receiver{ receiver, std::move(env) });
                        };
                        auto& op2 = ops_variant.template emplace<op2_t>(emplace_from{ make_op2 });

                        exec::start(op2);
                    }
                    catch (...) {
                        constexpr bool nothrow =
                            std::is_nothrow_invocable_v<decltype(invocable), std::decay_t<Ts>&...> &&
                            std::is_nothrow_invocable_v<connect_t,
                                                        std::invoke_result_t<decltype(invocable),
                                                                             std::decay_t<Ts>&...>,
                                                        ReceiverT> &&
                            (std::is_nothrow_constructible_v<std::decay_t<Ts>, Ts> && ...);

                        if constexpr (!nothrow) {
                            exec::set_error(std::move(receiver), std::current_exception());
                        }
                    }
                }
                else {
                    tag(std::move(receiver), std::forward<Ts>(args)...);
                }
            }

            constexpr let_state(SenderT&& sender, InvocableT invocable, ReceiverT& receiver) :
                invocable(std::move(invocable)),
                env(make_env(sender, exec::get_env(receiver))),
                ops_variant(std::in_place_type<op_t>, emplace_from{ [&]() { return exec::connect(std::forward<SenderT>(sender), inner_receiver{ *this, receiver }); } }) {}
        };

        template<typename SenderT, typename... EnvTs>
        [[nodiscard]] static consteval auto get_completion_signatures() {
            using invocable_t =
                decltype(std::forward_like<SenderT>(get_data(std::declval<SenderT>()).template get<0>()));
            using child_sender_t =
                decltype(std::forward_like<SenderT>(get_data(std::declval<SenderT>()).template get<1>()));
            using child_completion_signatures_t = completion_signatures_of_t<child_sender_t, EnvTs...>;

            using transformed =
                meta_add_t<gather_signatures<CompletionT,
                                             child_completion_signatures_t,
                                             meta_bind_front<signatures_t, invocable_t, type_holder<EnvTs...>>::template type,
                                             meta_bind_front<meta_add_t, completion_signatures<>>::type>,
                           meta_filter_t<CompletionT,
                                         child_completion_signatures_t,
                                         meta_not<has_same_tag>::type>>;

            using signatures_by_completion_t = meta_filter_t<CompletionT,
                                                             child_completion_signatures_t,
                                                             has_same_tag>;

            constexpr bool nothrow =
                is_nothrow_signatures<std::is_nothrow_invocable,
                                      signatures_by_completion_t,
                                      invocable_t> &&
                is_nothrow_signatures<nothrow_movable,
                                      signatures_by_completion_t> &&
                is_nothrow_signatures<nothrow_connectable,
                                      signatures_by_completion_t,
                                      invocable_t,
                                      dummy_receiver<meta_index_t<0, EnvTs...>>>;

            if constexpr (nothrow) {
                return meta_unique_t<transformed>{};
            }
            else {
                return meta_merge_t<transformed, completion_signatures<exec::set_error_t(std::exception_ptr)>>{};
            }
        }

        static constexpr auto get_state =
            []<typename SenderT, typename ReceiverT>(SenderT&& sender, ReceiverT& receiver) noexcept {
                auto&& invocable = get_data(std::forward<SenderT>(sender)).template get<0>();
                auto&& child = get_data(std::forward<SenderT>(sender)).template get<1>();

                using env_t =
                    decltype(make_env(std::declval<SenderT>(), exec::get_env(std::declval<ReceiverT>())));

                using child_sender_t = decltype(std::forward_like<SenderT>(child));
                using invocable_t = std::decay_t<decltype(invocable)>;

                using second_receiver_t = second_receiver<ReceiverT, env_t>;
                using child_completion_signatures_t =
                    completion_signatures_of_t<child_sender_t, env_of_t<ReceiverT>>;
                using args_variant_t =
                    meta_add_t<std::variant<std::monostate>,
                               gather_signatures<CompletionT,
                                                 child_completion_signatures_t,
                                                 decayed_tuple,
                                                 std::variant>>;
                using ops_variant_t =
                    meta_add_t<std::variant<std::monostate>,
                               gather_signatures<CompletionT,
                                                 child_completion_signatures_t,
                                                 meta_bind_front<as_op_t, invocable_t, second_receiver_t>::template type,
                                                 std::variant>>;

                using state_t = let_state<child_sender_t,
                                          invocable_t,
                                          ReceiverT,
                                          args_variant_t,
                                          ops_variant_t>;

                return state_t{ std::forward_like<SenderT>(child), std::forward_like<SenderT>(invocable), receiver };
            };

        static constexpr auto start =
            []<typename StateT, typename ReceiverT>(StateT& state, ReceiverT&) noexcept {
                exec::start(std::get<typename StateT::op_t>(state.ops_variant));
            };

    };

    template<typename>
    struct let_tag_t {
        template<typename SenderT, typename InvocableT>
        [[nodiscard]] constexpr auto operator()(SenderT&& input, InvocableT&& invocable) const noexcept {
            return details::make_sender(*this, details::product_type{ std::forward<InvocableT>(invocable), std::forward<SenderT>(input) });
        }

        template<typename InvocableT>
        [[nodiscard]] constexpr auto operator()(InvocableT&& invocable) const noexcept {
            return details::basic_closure{
                sender_adapter_closure<let_tag_t>{},
                details::product_type{ std::forward<InvocableT>(invocable) }
            };
        }
    };

    using let_value_t = let_tag_t<exec::set_value_t>;
    inline constexpr let_value_t let_value{};

    using let_error_t = let_tag_t<exec::set_error_t>;
    inline constexpr let_error_t let_error{};

    using let_stopped_t = let_tag_t<exec::set_stopped_t>;
    inline constexpr let_stopped_t let_stopped{};
}

#endif // !EXEC_LET_HPP