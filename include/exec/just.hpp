#ifndef EXEC_JUST_HPP
#define EXEC_JUST_HPP

#include "exec/completions.hpp"
#include "exec/completion_signatures.hpp"

#include "exec/details/basic_sender.hpp"
#include "exec/details/product_type.hpp"
#include "exec/details/type_list.hpp"

#include <type_traits>
#include <utility>

namespace exec {
    template<typename CompletionT>
    struct just_tag_t;

    template<typename CompletionT>
    struct details::impls_for<just_tag_t<CompletionT>> : default_impls {
        template<typename... Ts>
        using signature_t = completion_signatures<CompletionT(Ts...)>;

        static constexpr auto get_completion_signatures =
            []<typename SenderT, typename EnvT>(const SenderT& sender, const EnvT&) noexcept {
                return sender.apply([]<typename DataT>(auto&&, const DataT&, auto&&...) noexcept ->
                           elements_of<DataT>::template apply<signature_t>
                       {
                           return {};
                       });
            };

        static constexpr auto start =
            []<typename StateT, typename ReceiverT>(StateT& state, ReceiverT& receiver, auto&&...) noexcept {
                std::move(state).apply([&]<typename... Ts>(Ts&&... values) mutable noexcept {
                    CompletionT{}(std::move(receiver), std::forward<Ts>(values)...);
                });
            };
    };

    template<>
    struct just_tag_t<exec::set_value_t> {
        template<typename... Ts>
        [[nodiscard]] constexpr decltype(auto) operator()(Ts&&... values) const {
            return details::make_sender(*this, details::product_type{ std::forward<Ts>(values)... });
        }
    };
    using just_t = just_tag_t<exec::set_value_t>;
    inline constexpr just_t just{};

    template<>
    struct just_tag_t<exec::set_error_t> {
        template<typename T>
        [[nodiscard]] constexpr decltype(auto) operator()(T&& value) const {
            return details::make_sender(*this, details::product_type{ std::forward<T>(value) });
        }
    };
    using just_error_t = just_tag_t<exec::set_error_t>;
    inline constexpr just_error_t just_error{};

    template<>
    struct just_tag_t<exec::set_stopped_t> {
        [[nodiscard]] constexpr decltype(auto) operator()() const {
            return details::make_sender(*this, details::product_type{});
        }
    };
    using just_stopped_t = just_tag_t<exec::set_stopped_t>;
    inline constexpr just_stopped_t just_stopped{};
}

#endif // !EXEC_JUST_HPP