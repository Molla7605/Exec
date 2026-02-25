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

#include <concepts>
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
        template<typename SenderT>
        struct env {
            const SenderT& sender;

            [[nodiscard]] constexpr decltype(auto) query(get_completion_scheduler_t<CompletionT>) const noexcept
            requires std::invocable<get_completion_scheduler_t<CompletionT>, const SenderT&>
            {
                return get_completion_scheduler<CompletionT>(sender);
            }

            [[nodiscard]] constexpr empty_env query(get_completion_scheduler_t<CompletionT>) const noexcept {
                return {};
            }
        };

        template<typename ReceiverT, typename EnvT>
        struct second_receiver {
            using receiver_concept = exec::receiver_t;

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

            [[nodiscard]] constexpr decltype(auto) get_env() const noexcept {
                return join_env(env, forward_env(exec::get_env(receiver)));
            }
        };

        template<typename InvocableT, typename ReceiverT, typename... ArgTs>
        using as_op_t =
            connect_result_t<std::invoke_result_t<InvocableT, std::decay_t<ArgTs>&...>, ReceiverT>;

        template<typename InvocableT, typename EnvT, typename... ArgTs>
        using signatures_t =
            completion_signatures_of_t<std::invoke_result_t<InvocableT, std::decay_t<ArgTs>&...>, EnvT>;

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

        template<typename StateT, typename ReceiverT, typename... Ts>
        static void bind(StateT& state, ReceiverT& receiver, Ts&&... args) {
            using args_t = decayed_tuple<Ts...>;
            auto make_op = [&]() {
                return exec::connect(
                        std::apply(std::move(state.invocable),
                                   state.args.template emplace<args_t>(std::forward<Ts>(args)...)),
                        second_receiver{ receiver, std::move(state.env) }
                    );
            };

            exec::start(state.op.template emplace<decltype(make_op())>(emplace_from{ make_op }));
        }

        template<typename StateT, typename ReceiverT, typename... ArgTs>
        static void try_complete(StateT& state, ReceiverT& receiver, ArgTs&&... args) noexcept {
            constexpr bool nothrow =
                std::is_nothrow_invocable_v<decltype(state.invocable), std::decay_t<ArgTs>&...> &&
                std::is_nothrow_invocable_v<connect_t,
                                            std::invoke_result_t<decltype(state.invocable),
                                                                 std::decay_t<ArgTs>&...>,
                                            ReceiverT> &&
                (std::is_nothrow_constructible_v<std::decay_t<ArgTs>, ArgTs> && ...);

            try {
                bind(state, receiver, std::forward<ArgTs>(args)...);
            }
            catch (...) {
                if constexpr (!nothrow) {
                    exec::set_error(std::move(receiver), std::current_exception());
                }
            }
        }

        static constexpr auto get_completion_signatures =
            []<typename SenderT, typename EnvT>(SenderT&&, EnvT&&) noexcept {
                using child_sender_t =
                    decltype(std::forward_like<SenderT>(std::declval<child_of_t<SenderT, 0>>()));
                using invocable_t =
                    decltype(std::forward_like<SenderT>(std::declval<meta_index_of_t<1, std::decay_t<SenderT>>>()));
                using child_completion_signatures_t =
                    completion_signatures_of_t<child_sender_t, EnvT>;

                using transformed =
                    meta_add_t<
                        gather_signatures<CompletionT,
                                          child_completion_signatures_t,
                                          meta_bind_front<signatures_t, invocable_t, EnvT>::template type,
                                          meta_bind_front<meta_add_t, completion_signatures<>>::type>,
                        meta_filter_t<CompletionT,
                                      child_completion_signatures_t,
                                      meta_not<has_same_tag>::type>
                    >;

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
                                          invocable_t, dummy_receiver<EnvT>>;


                if constexpr (nothrow) {
                    return meta_unique_t<transformed>{};
                }
                else {
                    return meta_merge_t<transformed, completion_signatures<exec::set_error_t(std::exception_ptr)>>{};
                }
            };

        static constexpr auto get_state =
            []<typename SenderT, typename ReceiverT>(SenderT&& sender, ReceiverT& receiver) noexcept {
                using child_sender_t = child_of_t<SenderT, 0>;
                using env_t = env<child_sender_t>;
                using invocable_t = meta_index_of_t<1, std::decay_t<SenderT>>;
                using second_receiver_t = second_receiver<ReceiverT, env_t>;
                using child_completion_signatures_t =
                    completion_signatures_of_t<child_sender_t, env_of_t<ReceiverT>>;
                using args_t =
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

                struct state {
                    invocable_t invocable;
                    env_t env;
                    args_t args;
                    ops_variant_t op;
                };

                return state{
                    std::forward<SenderT>(sender).template get<1>(),
                    { sender.template get<2>() },
                    {},
                    {}
                };
            };

        static constexpr auto complete =
            []<typename StateT, typename ReceiverT, typename TagT, typename... ArgTs>
                (auto, StateT& state, ReceiverT& receiver, TagT, ArgTs&&... args) noexcept -> void
            {
                if constexpr (std::same_as<TagT, CompletionT>) {
                    try_complete(state, receiver, std::forward<ArgTs>(args)...);
                }
                else {
                    TagT{}(std::move(receiver), std::forward<ArgTs>(args)...);
                }
            };

    };

    template<typename CompletionT>
    struct let_tag_t {
        template<typename SenderT, typename InvocableT>
        [[nodiscard]] constexpr decltype(auto) operator()(SenderT&& input, InvocableT&& invocable) const noexcept {
            return details::make_sender(*this, std::forward<InvocableT>(invocable), std::forward<SenderT>(input));
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