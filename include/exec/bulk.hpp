#ifndef EXEC_BULK_HPP
#define EXEC_BULK_HPP

#include "exec/completions.hpp"
#include "exec/completion_signatures.hpp"
#include "exec/sender_adapter_closure.hpp"

#include "exec/details/basic_closure.hpp"
#include "exec/details/basic_sender.hpp"
#include "exec/details/gather_signatures.hpp"
#include "exec/details/is_nothrow_signatures.hpp"
#include "exec/details/meta_bind.hpp"
#include "exec/details/meta_filter.hpp"
#include "exec/details/meta_merge.hpp"
#include "exec/details/product_type.hpp"

#include <concepts>
#include <execution>
#include <type_traits>

namespace exec {
    struct bulk_chunked_t;

    template<>
    struct details::impls_for<bulk_chunked_t> : default_impls {
        template<typename InvocableT, typename ShapeT, typename... ArgTs>
        struct nothrow_invocable {
            static constexpr bool value = []() consteval {
                return std::is_nothrow_invocable_v<InvocableT,
                                                   decltype(auto(std::declval<ShapeT>())),
                                                   decltype(auto(std::declval<ShapeT>())),
                                                   ArgTs...>;
            }();
        };

        template<typename SenderT, typename... EnvTs>
        [[nodiscard]] static consteval auto get_completion_signatures() {
            using child_completion_signatures_t =
                completion_signatures_of_t<child_of_t<SenderT>, EnvTs...>;

            using invocable_t = decltype(std::declval<data_of_t<SenderT>>().template get<2>());
            using shape_t = decltype(std::declval<data_of_t<SenderT>>().template get<1>());

            constexpr bool nothrow =
                is_nothrow_signatures<meta_bind_front<nothrow_invocable, invocable_t, shape_t>::template type,
                                      meta_filter_t<exec::set_value_t, child_completion_signatures_t, has_same_tag>>;

            if constexpr (nothrow) {
                return meta_merge_t<child_completion_signatures_t,
                                    completion_signatures<exec::set_error_t(std::exception_ptr)>>{};
            }
            else {
                return child_completion_signatures_t{};
            }
        }

        static constexpr auto complete =
            []<typename StateT, typename ReceiverT, typename CompletionT, typename... ArgTs>
                (auto, StateT& state, ReceiverT& receiver, CompletionT, ArgTs&&... args) noexcept
                    requires std::invocable<decltype(state.template get<2>()),
                                            decltype(auto(state.template get<1>())),
                                            decltype(auto(state.template get<1>())),
                                            ArgTs...>
            {
                if constexpr (std::is_same_v<CompletionT, exec::set_value_t>) {
                    auto& policy = state.template get<0>();
                    auto& shape = state.template get<1>();
                    auto& invocable = state.template get<2>();

                    constexpr bool nothrow = noexcept(invocable(auto(shape), auto(shape), args...));
                    try {
                        invocable(static_cast<decltype(auto(shape))>(0), auto(shape), args...);
                        CompletionT{}(std::move(receiver), std::forward<ArgTs>(args)...);
                    }
                    catch (...) {
                        if constexpr (!nothrow) {
                            exec::set_error(std::move(receiver), std::current_exception());
                        }
                    }
                }
                else {
                    CompletionT{}(std::move(receiver), std::forward<ArgTs>(args)...);
                }
            };
    };

    struct bulk_chunked_t {
        template<typename SenderT, typename PolicyT, std::integral ShapeT, typename InvocableT>
        requires std::is_execution_policy_v<PolicyT>
        [[nodiscard]] constexpr auto operator()(SenderT&& sender, PolicyT policy, ShapeT shape, InvocableT&& invocable) const {
            return details::make_sender(*this,
                                        details::product_type{ policy, shape, std::forward<InvocableT>(invocable) },
                                        std::forward<SenderT>(sender));
        }

        template<typename PolicyT, std::integral ShapeT, typename InvocableT>
        requires std::is_execution_policy_v<PolicyT>
        [[nodiscard]] constexpr auto operator()(PolicyT policy, ShapeT shape, InvocableT&& invocable) const noexcept {
            return details::basic_closure{
                sender_adapter_closure<bulk_chunked_t>{},
                details::product_type{ policy, auto(shape), std::forward<InvocableT>(invocable) }
            };
        }
    };
    inline constexpr bulk_chunked_t bulk_chunked{};

    struct bulk_unchunked_t;

    template<>
    struct details::impls_for<bulk_unchunked_t> : default_impls {
        template<typename InvocableT, typename ShapeT, typename... ArgTs>
        struct nothrow_invocable {
            static constexpr bool value = []() consteval {
                return std::is_nothrow_invocable_v<InvocableT,
                                                   decltype(auto(std::declval<ShapeT>())),
                                                   ArgTs...>;
            }();
        };

        template<typename SenderT, typename... EnvTs>
        [[nodiscard]] static consteval auto get_completion_signatures() {
            using child_completion_signatures_t =
                completion_signatures_of_t<child_of_t<SenderT>, EnvTs...>;

            using invocable_t = decltype(std::declval<data_of_t<SenderT>>().template get<2>());
            using shape_t = decltype(std::declval<data_of_t<SenderT>>().template get<1>());

            constexpr bool nothrow =
                is_nothrow_signatures<meta_bind_front<nothrow_invocable, invocable_t, shape_t>::template type,
                                      meta_filter_t<exec::set_value_t, child_completion_signatures_t, has_same_tag>>;

            if constexpr (nothrow) {
                return meta_merge_t<child_completion_signatures_t,
                                    completion_signatures<exec::set_error_t(std::exception_ptr)>>{};
            }
            else {
                return child_completion_signatures_t{};
            }
        }

        static constexpr auto complete =
            []<typename IndexT, typename StateT, typename ReceiverT, typename CompletionT, typename... ArgTs>
                (IndexT, StateT& state, ReceiverT& receiver, CompletionT, ArgTs&&... args) noexcept
                    requires std::invocable<decltype(state.template get<2>()),
                                            decltype(auto(state.template get<1>())),
                                            ArgTs...>
            {
                if constexpr (std::is_same_v<CompletionT, exec::set_value_t>) {
                    auto& policy = state.template get<0>();
                    auto& shape = state.template get<1>();
                    auto& invocable = state.template get<2>();

                    constexpr bool nothrow = noexcept(invocable(auto(shape), args...));
                    try {
                        for (decltype(auto(shape)) count = 0; count < shape; ++count)
                            invocable(auto(count), args...);

                        CompletionT{}(std::move(receiver), std::forward<ArgTs>(args)...);
                    }
                    catch (...) {
                        if constexpr (!nothrow) {
                            exec::set_error(std::move(receiver), std::current_exception());
                        }
                    }
                }
                else {
                    CompletionT{}(std::move(receiver), std::forward<ArgTs>(args)...);
                }
            };
    };

    struct bulk_unchunked_t {
        template<typename SenderT, typename PolicyT, std::integral ShapeT, typename InvocableT>
        requires std::is_execution_policy_v<PolicyT>
        [[nodiscard]] constexpr auto operator()(SenderT&& sender, PolicyT policy, ShapeT shape, InvocableT&& invocable) noexcept {
            return details::make_sender(*this,
                                        details::product_type{ policy, shape, std::forward<InvocableT>(invocable) },
                                        std::forward<SenderT>(sender));
        }

        template<typename PolicyT, std::integral ShapeT, typename InvocableT>
        requires std::is_execution_policy_v<PolicyT>
        [[nodiscard]] constexpr auto operator()(PolicyT policy, ShapeT shape, InvocableT&& invocable) const noexcept {
            return details::basic_closure{
                sender_adapter_closure<bulk_unchunked_t>{},
                details::product_type{ policy, auto(shape), std::forward<InvocableT>(invocable) }
            };
        }
    };
    inline constexpr bulk_unchunked_t bulk_unchunked{};
}

#endif // !EXEC_BULK_HPP
