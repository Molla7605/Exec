#ifndef EXEC_DETAILS_BASIC_SENDER_HPP
#define EXEC_DETAILS_BASIC_SENDER_HPP

#include "exec/completions.hpp"
#include "exec/env.hpp"
#include "exec/operation_state.hpp"
#include "exec/receiver.hpp"
#include "exec/sender.hpp"

#include "exec/details/forward_env.hpp"
#include "exec/details/product_type.hpp"

#include <concepts>
#include <type_traits>
#include <utility>

namespace exec::details {
    template<typename T>
    using tag_of_t = std::remove_cvref_t<decltype(std::declval<T>().template get<0>())>;

    struct default_impls {
        static constexpr auto get_attrs =
            []<typename... ChildTs>(const auto&, const ChildTs&... children) noexcept -> decltype(auto) {
                if constexpr (sizeof...(ChildTs) == 1) {
                    return (forward_env(exec::get_env(children)), ...);
                }
                else {
                    return empty_env{};
                }
            };

        static constexpr auto get_env =
            []<typename ReceiverT>(auto, auto&, const ReceiverT& receiver) noexcept -> decltype(auto) {
                return forward_env(exec::get_env(receiver));
            };

        static constexpr auto get_state =
            []<typename SenderT, typename ReceiverT>(SenderT&& sender, ReceiverT& receiver) noexcept -> decltype(auto) {
                return std::forward<SenderT>(sender).apply(
                    [&]<typename DataT>(auto&&, DataT&& data, auto&&...) mutable noexcept -> decltype(auto) {
                        return std::forward<DataT>(data);
                    });
                };

        static constexpr auto start =
            []<typename... ArgTs>(auto&, auto&, ArgTs&&... operations) noexcept -> void {
                (exec::start(operations), ...);
            };

        static constexpr auto complete =
            []<typename IndexT, typename ReceiverT, typename TagT, typename... ArgTs>
                (IndexT, auto&, ReceiverT& receiver, TagT, ArgTs&&... args) noexcept ->
                    void requires std::invocable<TagT, ReceiverT, ArgTs...>
            {
                TagT{}(std::move(receiver), std::forward<ArgTs>(args)...);
            };
    };

    template<typename TagT>
    struct impls_for : default_impls {};

    template<typename SenderT, typename ReceiverT>
    using state_from_tag_t =
        std::decay_t<std::invoke_result_t<decltype(impls_for<tag_of_t<SenderT>>::get_state), SenderT, ReceiverT&>>;

    template<typename IndexT, typename SenderT, typename ReceiverT>
    using env_from_tag_t =
        std::invoke_result_t<decltype(impls_for<tag_of_t<SenderT>>::get_env), IndexT, state_from_tag_t<SenderT, ReceiverT>&, const ReceiverT&>;

    template<typename SenderT, std::size_t INDEX = 0>
    using child_of_t = decltype(std::declval<SenderT>().template get<INDEX + 2>());

    template<typename SenderT>
    using indices_for = std::remove_reference_t<SenderT>::indices_for;

    template<typename SenderT, typename ReceiverT>
    struct basic_state {
        basic_state(SenderT&& sndr, ReceiverT&& rcvr)
            noexcept(std::is_nothrow_move_constructible_v<ReceiverT> &&
                     std::is_nothrow_invocable_v<decltype(impls_for<tag_of_t<SenderT>>::get_state), SenderT, ReceiverT&>) :
                receiver(std::move(rcvr)),
                state(impls_for<tag_of_t<SenderT>>::get_state(std::forward<SenderT>(sndr), receiver)) {}

        ReceiverT receiver;
        state_from_tag_t<SenderT, ReceiverT> state;
    };

    template<typename SenderT, typename ReceiverT, typename IndexT>
    struct basic_receiver {
        using receiver_concept = exec::receiver_t;

        using tag_t = tag_of_t<SenderT>;
        using state_t = state_from_tag_t<SenderT, ReceiverT>;

        static constexpr const auto& complete = impls_for<tag_t>::complete;

        basic_state<SenderT, ReceiverT>* op;

        template<typename... Ts>
        requires std::invocable<decltype(complete), IndexT, state_t&, ReceiverT&, exec::set_value_t, Ts...>
        void set_value(Ts&&... values) && noexcept {
            complete(IndexT(), op->state, op->receiver, exec::set_value_t{}, std::forward<Ts>(values)...);
        }

        template<typename T>
        requires std::invocable<decltype(complete), IndexT, state_t&, ReceiverT&, exec::set_error_t, T>
        void set_error(T&& value) && noexcept {
            complete(IndexT(), op->state, op->receiver, exec::set_error_t{}, std::forward<T>(value));
        }

        void set_stopped() && noexcept
            requires std::invocable<decltype(complete), IndexT, state_t&, ReceiverT&, exec::set_stopped_t>
        {
            complete(IndexT(), op->state, op->receiver, exec::set_stopped_t{});
        }

        [[nodiscard]] constexpr env_from_tag_t<IndexT, SenderT, ReceiverT> get_env() const noexcept {
            return impls_for<tag_t>::get_env(IndexT(), op->state, op->receiver);
        }
    };

    constexpr auto connect_all =
        []<typename SenderT, typename ReceiverT, std::size_t... INDICES>
            (basic_state<SenderT, ReceiverT>* state, SenderT&& sender, std::index_sequence<INDICES...>) noexcept -> decltype(auto)
    {
        return std::forward<SenderT>(sender).apply([&]<typename... ChildTs>(auto, auto&&, ChildTs&&... children) mutable noexcept -> decltype(auto) {
            return product_type{
                exec::connect(std::forward<ChildTs>(children),
                              basic_receiver<SenderT, ReceiverT, std::integral_constant<std::size_t, INDICES>>{ state })...
            };
        });
    };

    template<typename SenderT, typename ReceiverT>
    using connect_all_result_t =
        std::invoke_result_t<decltype(connect_all), basic_state<SenderT, ReceiverT>*, SenderT, indices_for<SenderT>>;

    template<typename SenderT, typename ReceiverT>
    struct basic_operation : basic_state<SenderT, ReceiverT> {
        using operation_state_concept = exec::operation_state_t;
        using tag_t = tag_of_t<SenderT>;

        connect_all_result_t<SenderT, ReceiverT> inner;

        basic_operation(SenderT&& sender, ReceiverT&& receiver)
            noexcept(std::is_nothrow_constructible_v<basic_state<SenderT, ReceiverT>, SenderT, ReceiverT> &&
                     noexcept(connect_all(this, std::forward<SenderT>(sender), indices_for<SenderT>{}))) :
                basic_state<SenderT, ReceiverT>(std::forward<SenderT>(sender), std::move(receiver)),
                inner(connect_all(this, std::forward<SenderT>(sender), indices_for<SenderT>{})) {}

        void start() & noexcept {
            inner.apply([this]<typename... ArgTs>(ArgTs&... args) mutable noexcept {
                impls_for<tag_t>::start(this->state, this->receiver, args...);
            });
        }

    };

    template<typename TagT, typename DataT, typename... ChildTs>
    struct basic_sender : product_type<TagT, DataT, ChildTs...> {
        using sender_concept = exec::sender_t;
        using indices_for = std::index_sequence_for<ChildTs...>;

        [[nodiscard]] constexpr decltype(auto) get_env() const noexcept {
            return this->apply([](auto&&, const DataT& data, const ChildTs&... children) noexcept -> decltype(auto) {
                return impls_for<TagT>::get_attrs(data, children...);
            });
        }

        template<typename Self, typename ReceiverT>
        [[nodiscard]] constexpr basic_operation<Self, ReceiverT> connect(this Self&& self, ReceiverT receiver)
            noexcept(std::is_nothrow_constructible_v<basic_operation<Self, ReceiverT>, Self, ReceiverT>)
        {
            return { std::forward<Self>(self), std::move(receiver) };
        }

        template<typename Self, typename EnvT>
        [[nodiscard]] constexpr decltype(auto) get_completion_signatures(this Self&& self, EnvT&& env) noexcept {
            return impls_for<TagT>::get_completion_signatures(std::forward<Self>(self), std::forward<EnvT>(env));
        }

    };

    template<typename TagT, typename DataT, typename... ChildTs>
    constexpr auto make_sender(TagT tag, DataT&& data, ChildTs&&... children) ->
        basic_sender<TagT, std::decay_t<DataT>, std::decay_t<ChildTs>...>
    {
        return { tag, std::forward<DataT>(data), std::forward<ChildTs>(children)... };
    }
}

#endif // !EXEC_DETAILS_BASIC_SENDER_HPP