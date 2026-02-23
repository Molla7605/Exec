#ifndef EXEC_THEN_HPP
#define EXEC_THEN_HPP

#include "exec/completions.hpp"
#include "exec/completion_signatures.hpp"
#include "exec/sender.hpp"
#include "exec/sender_adapter_closure.hpp"

#include "exec/details/basic_closure.hpp"
#include "exec/details/basic_sender.hpp"
#include "exec/details/conditional_meta_apply.hpp"
#include "exec/details/gather_signatures.hpp"
#include "exec/details/is_nothrow_signatures.hpp"
#include "exec/details/meta_add.hpp"
#include "exec/details/meta_bind.hpp"
#include "exec/details/meta_filter.hpp"
#include "exec/details/meta_index.hpp"
#include "exec/details/meta_merge.hpp"
#include "exec/details/meta_not.hpp"
#include "exec/details/type_holder.hpp"
#include "exec/details/unique_template.hpp"

#include <concepts>
#include <exception>
#include <type_traits>
#include <utility>

namespace exec {
    template<typename CompletionT>
    struct then_tag_t {};

    template<typename CompletionT>
    struct details::impls_for<then_tag_t<CompletionT>> : default_impls {
        template<typename... Args>
        using set_value_signature_t = exec::set_value_t(Args...);

        template<typename InvocableT, typename... ArgTs>
        using signature_t = conditional_meta_apply_t<std::is_void_v<std::invoke_result_t<InvocableT, ArgTs...>>,
                                                     set_value_signature_t,
                                                     type_holder<>,
                                                     type_holder<std::invoke_result_t<InvocableT, ArgTs...>>>;

        template<typename InvocableT, typename... ArgTs>
        using is_nothrow_t = std::bool_constant<std::is_nothrow_invocable_v<InvocableT, ArgTs...>>;

        template<typename InvocableT, typename ReceiverT, typename... ArgTs>
        static constexpr void try_complete(InvocableT&& invocable, ReceiverT& receiver, ArgTs&&... args) noexcept {
            constexpr bool nothrow = std::is_nothrow_invocable_v<InvocableT, ArgTs...>;
            try {
                if constexpr (std::is_void_v<std::invoke_result_t<InvocableT, ArgTs...>>) {
                    std::invoke(std::forward<InvocableT>(invocable), std::forward<ArgTs>(args)...);
                    exec::set_value(std::move(receiver));
                }
                else {
                    exec::set_value(std::move(receiver),
                                    std::invoke(std::forward<InvocableT>(invocable), std::forward<ArgTs>(args)...));
                }
            }
            catch (...) {
                if constexpr (!nothrow) {
                    exec::set_error(std::move(receiver), std::current_exception());
                }
            }
        }

        static constexpr auto get_completion_signatures = []<typename SenderT, typename EnvT>(SenderT&&, EnvT&&) noexcept {
            using child_sender_t =
                decltype(std::forward_like<SenderT>(std::declval<child_of_t<SenderT, 0>>()));
            using invocable_t =
                decltype(std::forward_like<SenderT>(std::declval<meta_index_of_t<1, std::decay_t<SenderT>>>()));

            using child_completion_signatures_t = completion_signatures_of_t<child_sender_t, EnvT>;

            using transformed =
                meta_add_t<gather_signatures<CompletionT,
                                             child_completion_signatures_t,
                                             meta_bind_front<signature_t, invocable_t>::template type,
                                             completion_signatures>,
                           meta_filter_t<CompletionT,
                                         child_completion_signatures_t,
                                         meta_not<has_same_tag>::type>>;

            constexpr bool nothrow =
                is_nothrow_signatures<invocable_t, meta_filter_t<CompletionT, child_completion_signatures_t, has_same_tag>>;

            if constexpr (nothrow) {
                return meta_unique_t<transformed>{};
            }
            else {
                return meta_merge_t<transformed,
                                    completion_signatures<exec::set_error_t(std::exception_ptr)>>{};
            }
        };

        static constexpr auto complete =
            []<typename InvocableT, typename ReceiverT, typename TagT, typename... ArgTs>
                (auto, InvocableT&& invocable, ReceiverT& receiver, TagT, ArgTs&&... args) noexcept -> void
            {
                if constexpr (std::is_same_v<CompletionT, TagT>) {
                    try_complete(std::forward<InvocableT>(invocable), receiver, std::forward<ArgTs>(args)...);
                }
                else {
                    TagT{}(std::move(receiver), std::forward<ArgTs>(args)...);
                }
            };
    };

    template<>
    struct then_tag_t<exec::set_value_t> {
        template<sender SenderT, typename InvocableT>
        [[nodiscard]] constexpr auto operator()(SenderT input, InvocableT&& invocable) const {
            return details::make_sender(*this, std::forward<InvocableT>(invocable), std::forward<SenderT>(input));
        }

        template<typename InvocableT>
        [[nodiscard]] constexpr auto operator()(InvocableT&& invocable) const {
            return details::basic_closure{
                sender_adapter_closure<then_tag_t>{},
                details::product_type{ std::forward<InvocableT>(invocable) }
            };
        }
    };
    using then_t = then_tag_t<exec::set_value_t>;
    inline constexpr then_t then{};

    template<>
    struct then_tag_t<exec::set_error_t> {
        template<sender SenderT, typename InvocableT>
        [[nodiscard]] constexpr auto operator()(SenderT input, InvocableT&& invocable) const {
            return details::make_sender(*this, std::forward<InvocableT>(invocable), std::forward<SenderT>(input));
        }

        template<typename InvocableT>
        [[nodiscard]] constexpr auto operator()(InvocableT&& invocable) const {
            return details::basic_closure{
                sender_adapter_closure<then_tag_t>{},
                details::product_type{ std::forward<InvocableT>(invocable) }
            };
        }
    };
    using upon_error_t = then_tag_t<exec::set_error_t>;
    inline constexpr upon_error_t upon_error{};

    template<>
    struct then_tag_t<exec::set_stopped_t> {
        template<sender SenderT, std::invocable InvocableT>
        [[nodiscard]] constexpr auto operator()(SenderT input, InvocableT&& invocable) const {
            return details::make_sender(*this, std::forward<InvocableT>(invocable), std::forward<SenderT>(input));
        }

        template<typename InvocableT>
        [[nodiscard]] constexpr auto operator()(InvocableT&& invocable) const {
            return details::basic_closure{
                sender_adapter_closure<then_tag_t>{},
                details::product_type{ std::forward<InvocableT>(invocable) }
            };
        }
    };
    using upon_stopped_t = then_tag_t<exec::set_stopped_t>;
    inline constexpr upon_stopped_t upon_stopped{};
}

#endif // !EXEC_THEN_HPP