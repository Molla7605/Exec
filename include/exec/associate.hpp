#ifndef EXEC_ASSOCIATE_HPP
#define EXEC_ASSOCIATE_HPP

#include "exec/completions.hpp"
#include "exec/completion_signatures.hpp"
#include "exec/env.hpp"
#include "exec/operation_state.hpp"
#include "exec/scope_token.hpp"
#include "exec/sender.hpp"
#include "exec/sender_adapter_closure.hpp"

#include "exec/details/basic_closure.hpp"
#include "exec/details/emplace_from.hpp"
#include "exec/details/basic_sender.hpp"
#include "exec/details/product_type.hpp"

#include <concepts>
#include <optional>
#include <utility>
#include <variant>

namespace exec {
    struct associate_t;

    template<>
    struct details::impls_for<associate_t> : default_impls {
        template<typename TokenT, typename SenderT>
        struct data {
            using wrapped_sender_t =
                std::remove_cvref_t<decltype(std::declval<TokenT>().wrap(std::declval<SenderT>()))>;

            using assoc_t = std::decay_t<decltype(std::declval<TokenT>().try_associate())>;

            using product_t = product_type<assoc_t, wrapped_sender_t>;

            std::optional<wrapped_sender_t> sender_opt;
            assoc_t association;

            explicit data(TokenT token, SenderT&& sndr) :
                sender_opt(token.wrap(std::forward<SenderT>(sndr))),
                association(token.try_associate())
            {
                if (!association) {
                    sender_opt.reset();
                }
            }

            data(const data& other) noexcept(std::is_nothrow_copy_constructible_v<wrapped_sender_t> &&
                                             noexcept(other.association.try_associate()))
                requires std::copy_constructible<wrapped_sender_t> :
                    sender_opt(other.sender_opt),
                    association(other.association.try_associate()) {}

            data(data&& other) noexcept(std::is_nothrow_move_constructible_v<wrapped_sender_t>) :
                sender_opt(std::move(other.sender_opt)),
                association(std::move(other.association)) {}

            product_t release() && noexcept {
                return product_type{ std::move(association), std::move(sender_opt).value() };
            }

        };

        // template<typename TokenT, typename SenderT>
        // data(TokenT, SenderT&&) -> data<std::decay_t<TokenT>, std::decay_t<SenderT>>;

        static constexpr auto get_attrs =
            [](const auto&, const auto&...) noexcept -> empty_env {
                return {};
            };

        static constexpr auto get_completion_signatures =
            []<typename SenderT, typename EnvT>(SenderT&&, EnvT&&) noexcept {
                using data_t = std::remove_cvref_t<data_of_t<SenderT>>;

                using wrapped_sender_t =
                    decltype(std::forward_like<SenderT>(std::declval<typename data_t::wrapped_sender_t>()));

                return completion_signatures_of_t<wrapped_sender_t, EnvT>{};
            };

        static constexpr auto get_state =
            []<typename SenderT, typename ReceiverT>(SenderT&& sender, ReceiverT& receiver)
                noexcept((std::is_same_v<SenderT, std::remove_cvref_t<SenderT>> ||
                          std::is_nothrow_constructible_v<std::remove_cvref_t<SenderT>, SenderT>) &&
                         std::is_nothrow_invocable_v<connect_t, typename std::remove_cvref_t<data_of_t<SenderT>>::wrapped_sender_t, ReceiverT>)
            {
                auto&& data = get_data(std::forward<SenderT>(sender));

                struct state {
                    using data_t = std::decay_t<data_of_t<SenderT>>;
                    using assoc_t = data_t::assoc_t;

                    using op_t = connect_result_t<typename data_t::wrapped_sender_t, ReceiverT>;

                    assoc_t association;
                    std::variant<std::decay_t<ReceiverT>*, op_t> data;

                    explicit state(data_t::product_t data, ReceiverT& receiver) :
                        association(std::move(data).template get<0>())
                    {
                        if (association) {
                            auto make_op = [&]() -> decltype(auto) {
                                return exec::connect(std::move(data).template get<1>(), receiver);
                            };

                            this->data.template emplace<decltype(make_op())>(emplace_from{ make_op });
                        }
                        else {
                            this->data.template emplace<std::decay_t<ReceiverT>*>(std::addressof(receiver));
                        }
                    }

                    explicit state(data_t&& data, ReceiverT& receiver) :
                        state(std::move(data).release(), receiver) {}

                    explicit state(const data_t& data, ReceiverT& receiver)
                        requires std::is_copy_constructible_v<data_t> :
                            state(data_t(data).release(), receiver) {}

                    state(state&&) = delete;

                    void run() noexcept {
                        if (association) {
                            exec::start(std::get<op_t>(data));
                        }
                        else {
                            auto* const rcvr = std::get<std::decay_t<ReceiverT>*>(data);
                            exec::set_stopped(std::move(*rcvr));
                        }
                    }

                };

                return state{ std::forward_like<SenderT>(data), receiver };
            };

        static constexpr auto start =
            []<typename StateT>(StateT& state, auto&&) noexcept {
                state.run();
            };
    };

    struct associate_t {
        template<sender SenderT, scope_token TokenT>
        [[nodiscard]] constexpr auto operator()(SenderT&& input, TokenT token) const {
            using data_t =
                details::impls_for<associate_t>::data<std::decay_t<TokenT>, std::decay_t<SenderT>>;

            return details::make_sender(*this, data_t(token, std::forward<SenderT>(input)));
        }

        template<scope_token TokenT>
        [[nodiscard]] constexpr auto operator()(TokenT token) const
            noexcept(std::is_nothrow_copy_constructible_v<TokenT>)
        {
            return details::basic_closure{
                sender_adapter_closure<associate_t>{},
                details::product_type{ std::forward<TokenT>(token) }
            };
        }
    };
    inline constexpr associate_t associate{};
}

#endif // !EXEC_ASSOCIATE_HPP