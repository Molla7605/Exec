#ifndef EXEC_DETAILS_STOP_WHEN_HPP
#define EXEC_DETAILS_STOP_WHEN_HPP

#include "exec/sender.hpp"
#include "exec/stop_token.hpp"

#include "exec/details/basic_sender.hpp"
#include "exec/details/write_env.hpp"

namespace exec::details {
    struct stop_when_t;

    template<typename TokenT, typename CallbackT>
    class join_callback;

    template<typename SenderT, typename ReceiverT>
    class join_token {
    public:
        using stoken_t = stop_token_of_t<env_of_t<SenderT>>;
        using rtoken_t = stop_token_of_t<env_of_t<ReceiverT>>;

        template<typename CallbackT>
        using callback_type = join_callback<join_token, CallbackT>;

        explicit join_token(const SenderT& sender, const ReceiverT& receiver) :
            m_stoken(get_stop_token(exec::get_env(sender))),
            m_rtoken(get_stop_token(exec::get_env(receiver))) {}

        [[nodiscard]] consteval bool stop_possible() const noexcept {
            return m_stoken.stop_possible() || m_rtoken.stop_possible();
        }

        [[nodiscard]] bool stop_requested() const noexcept {
            return m_stoken.stop_requested() || m_rtoken.stop_requested();
        }

    private:
        template<typename TokenT, typename CallbackT>
        friend class join_callback;

        template<typename CallbackT>
        using data_t =
            product_type<typename stoken_t::template callback_type<CallbackT>,
                         typename rtoken_t::template callback_type<CallbackT>>;

        template<typename CallbackT>
        [[nodiscard]] constexpr data_t<CallbackT> register_callback(const CallbackT& callback) {
            return {
                stoken_t::template callback_type<CallbackT>(callback),
                rtoken_t::template callback_type<CallbackT>(callback)
            };
        }

        stoken_t m_stoken;
        rtoken_t m_rtoken;

    };

    template<typename TokenT, typename CallbackT>
    class join_callback {
    public:
        template<typename InitT>
        explicit join_callback(TokenT token, InitT&& init)
            noexcept(std::is_nothrow_copy_constructible_v<TokenT> &&
                     std::is_nothrow_constructible_v<CallbackT, InitT>) :
                m_data(token.register_callback([this] { invoke(); })),
                m_callback(std::forward<InitT>(init)),
                m_callback_ref(&m_callback) {}

        join_callback(const join_callback&) = delete;
        join_callback& operator=(const join_callback&) = delete;

        join_callback(join_callback&&) = delete;
        join_callback& operator=(join_callback&&) = delete;

    private:
        void invoke() noexcept {
            auto* const callback = m_callback_ref.exchange(nullptr, std::memory_order_relaxed);

            if (callback != nullptr) {
                std::invoke(*callback);
            }
        }

        TokenT::data_t m_data;
        CallbackT m_callback;
        std::atomic<CallbackT*> m_callback_ref;

    };

    template<>
    struct impls_for<stop_when_t> : default_impls {
        static constexpr auto get_completion_signatures =
            []<typename SenderT, typename EnvT>(SenderT&&, EnvT&&) noexcept {
                return transform_completion_signatures_of<child_of_t<SenderT>,
                                                          EnvT,
                                                          completion_signatures<exec::set_stopped_t()>>{};
            };

        static constexpr auto get_state =
            []<typename SenderT, typename ReceiverT>(SenderT&& sender, ReceiverT& receiver) noexcept {
                if constexpr (unstoppable_token<env_of_t<ReceiverT>>) {
                    return exec::connect(
                        write_env(get_child<0>(std::forward<SenderT>(sender)),
                                  prop{ get_stop_token, get_data(std::forward<SenderT>(sender)) }),
                        std::move(receiver)
                    );
                }
                else {
                    return exec::connect(
                        write_env(get_child<0>(std::forward<SenderT>(sender)),
                                  prop{ get_stop_token, join_token<SenderT, ReceiverT>(sender, receiver) }),
                        std::move(receiver)
                    );
                }
            };

        static constexpr auto start =
            []<typename StateT>(StateT& state, auto&, auto&&...) {
                exec::start(state);
            };
    };

    struct stop_when_t {
        template<sender SenderT, unstoppable_token TokenT>
        [[nodiscard]] constexpr decltype(auto) operator()(SenderT&& sender, TokenT&&) const noexcept {
            return std::forward<SenderT>(sender);
        }

        template<sender SenderT, stoppable_token TokenT>
        [[nodiscard]] constexpr auto operator()(SenderT&& sender, TokenT&& token) const {
            return details::make_sender(*this, std::forward<TokenT>(token), std::forward<SenderT>(sender));
        }
    };
    inline constexpr stop_when_t stop_when{};
}

#endif // !EXEC_DETAILS_STOP_WHEN_HPP